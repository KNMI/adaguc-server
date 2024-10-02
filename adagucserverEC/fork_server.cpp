#include <map>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CTString.h"
#include "fork_server.h"

static std::map<pid_t, int> child_sockets;

int get_max_pending_connections() {
  // Set the max number of queued connections on the socket via ADAGUC_NUMPARALLELPROCESSES
  CT::string num_parallel_processes(getenv("ADAGUC_NUMPARALLELPROCESSES"));
  int max_pending_connections;
  if (num_parallel_processes.isInt()) {
    return num_parallel_processes.toInt();
  }

  // Default to 4 queued connections
  return 4;
}

void handle_client(int client_socket, int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp) {
  int recv_buf_len = 4096;
  char recv_buf[recv_buf_len];
  memset(recv_buf, 0, recv_buf_len * sizeof(char));

  int data_recv = recv(client_socket, recv_buf, recv_buf_len, 0);
  if (data_recv > 0) {
    // The child stdout should end up in the client socket
    dup2(client_socket, STDOUT_FILENO);

    setenv("QUERY_STRING", recv_buf, 1);

    int status = run_adaguc_once(argc, argv, envp, true);
    // fprintf(stderr, "exiting, status=%d", status);

    // fflush(stdout);
    // fflush(stderr);

    exit(0);
  }
}

void child_signal_handler(int child_signal) {
  /*
  This function gets executed once a child exits (normally or through a signal)
  The kernel does not queue signals. If a child exits during the handling of another signal, the exit/signal gets dropped.
  Calling `waitpid` with WNOHANG solves this, it will loop over all exited children and return if none are found.

  This function should not use any non-reentrant calls (i.e. no `printf`) as this blocks the handler.
  */

  int stat_val;
  pid_t child_pid;

  // Loop over all exited children. WNOHANG makes the call non-blocking.
  while ((child_pid = waitpid(-1, &stat_val, WNOHANG)) > 0) {
    int child_status;

    if (WIFEXITED(stat_val)) {
      // child process exited normally. Collect child exit code (should be 0)
      child_status = WEXITSTATUS(stat_val);
    } else if (WIFSIGNALED(stat_val)) {
      // child process terminated due to signal (e.g. SIGSEGV). Set child exit code to signal.
      child_signal = WTERMSIG(stat_val);
      child_status = child_signal;
    } else {
      // not sure what to do here...
      child_status = 1;
    }

    int child_sock = child_sockets[child_pid];
    // fprintf(stderr, "@ on_child_exit [%d] [%d] [%d] [%d]\n", child_pid, child_sock, child_signal, child_status);

    // Write the status code from the child pid into the unix socket back to python
    write(child_sock, reinterpret_cast<const char *>(&child_status), sizeof(child_status));
    close(child_sock);

    child_sockets.erase(child_pid);
  }
}

int run_as_fork_service(int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp) {
  /*
  Start adaguc in fork mode. This means:
  - Set up a signal handler for any child processes
  - Set up a socket through system calls: `socket`, `bind`, `listen`
  - While true, accept any incoming connections to the socket, through system call `accept`
  - If connected, fork this process.
  -   Child process will handle further communication through the `client_socket`, will handle the adaguc request and exit normally.
  -   Parent process keeps running, track of the created `client_socket` and check for new incoming connections.
  */

  int client_socket = 0;

  // Create a signal handler for all children (all received SIGCHLD signals)
  // Signal mask is empty, meaning no additional signals are blocked while the handler is executed
  struct sigaction sa;
  sa.sa_handler = child_signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, NULL);

  // Create an endpoint for communicating through a unix socket
  int listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == listen_socket) {
    printf("Error on socket() call \n");
    return 1;
  }

  struct sockaddr_un local, remote;
  int len = 0;
  local.sun_family = AF_UNIX;

  CT::string socket_path(getenv("ADAGUC_PATH"));
  socket_path.concat("/adaguc.socket");

  strncpy(local.sun_path, socket_path.c_str(), sizeof(local.sun_path));
  local.sun_path[sizeof(local.sun_path) - 1] = '\0';

  // Remove old adaguc.socket file
  unlink(local.sun_path);

  // Bind name to the local socket, this will create an entry in the filesystem
  len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
  if (bind(listen_socket, (struct sockaddr *)&local, len) != 0) {
    printf("Error on binding socket \n");
    return 1;
  }

  // Start listening on the socket. Can have `max_pending_connections` number of connections queued.
  if (listen(listen_socket, get_max_pending_connections()) != 0) {
    printf("Error on listen call \n");
    return 1;
  }

  while (1) {
    unsigned int sock_len = 0;

    // Once someone connects to the unix socket, immediately fork and execute the client request in `handle_client`
    if ((client_socket = accept(listen_socket, (struct sockaddr *)&remote, &sock_len)) == -1) {
      printf("Error on accept() call \n");
      return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
      // Child process handles request. Communication with python happens through `client_socket`
      close(listen_socket);
      handle_client(client_socket, run_adaguc_once, argc, argv, envp);
    } else {
      // Parent process keeps track of new socket and returns to listen for new connections
      child_sockets[pid] = client_socket;
    }
  }

  return 0;
}