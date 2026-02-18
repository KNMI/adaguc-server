#include <map>
#include <mutex>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "CDebugger.h"
#include "CTString.h"
#include "fork_server.h"

// Check this many seconds for old left-over processes
const int CHECK_CHILD_PROC_INTERVAL = 30;
// Allow a backlog of this many connections. Uses `ADAGUC_NUMPARALLELPROCESSES` env var.
const int DEFAULT_QUEUED_CONNECTIONS = 4 * 32;
// Old left-over child process should be killed after this many seconds. Uses `ADAGUC_MAX_PROC_TIMEOUT` env var.
// const int DEFAULT_MAX_PROC_TIMEOUT = 300;
const int MAX_CHILD_PROC_TIMEOUT = 120;


int sigchld_pipe[2];

int get_env_var_int(const char *env, int default_val) {
  CT::string env_var(getenv(env));
  return env_var.isInt() ? env_var.toInt() : default_val;
}

void handle_client(int client_socket, int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp) {
  /*
  Read bytes from client socket with the assumption that it is the raw QUERY_STRING value.
  Set the QUERY_STRING and handle the request normally.
  */

  int recv_buf_len = 4096;
  char recv_buf[recv_buf_len];
  memset(recv_buf, 0, recv_buf_len * sizeof(char));

  int data_recv = recv(client_socket, recv_buf, recv_buf_len, 0);
  if (data_recv <= 0) {
    _exit(1);
  }

  // ---- Health check fast path ----
  // fprintf(stderr, "recv: %s\n", recv_buf);
  if (strncmp(recv_buf, "PING", 4) == 0) {
      const char *resp = "PONG\n";
      send(client_socket, resp, strlen(resp), 0);
      exit(0);
  }

  // The child stdout should end up in the client socket
  dup2(client_socket, STDOUT_FILENO);

  setenv("QUERY_STRING", recv_buf, 1);

  int status = run_adaguc_once(argc, argv, envp, true);
  fprintf(stderr, "exiting, status=%d\n", status);

  // fflush(stdout);
  // fflush(stderr);

  _exit(status);
}

void child_signal_handler(int) {
  /*
  This function gets executed once a child exits (normally or through a signal)
  The kernel does not queue signals. If a child exits during the handling of another signal, the exit/signal gets dropped.
  Calling `waitpid` with WNOHANG solves this, it will loop over all exited children and return if none are found.

  This function should not use any non-reentrant calls (i.e. no printf) as this blocks the handler.
  */

  pid_t pid;
  int status;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // Write both pid and raw status to pipe
    write(sigchld_pipe[1], &pid, sizeof(pid));
    write(sigchld_pipe[1], &status, sizeof(status));
  }
}

void handle_child_exit_events() {
  while (1) {
    pid_t exited_pid;
    int status;

    ssize_t r = read(sigchld_pipe[0], &exited_pid, sizeof(exited_pid));
    if (r <= 0)
      break;

    read(sigchld_pipe[0], &status, sizeof(status));

    auto it = child_procs.find(exited_pid);
    if (it != child_procs.end()) {

      int child_status;

      if (WIFEXITED(status))
        child_status = WEXITSTATUS(status);
      else if (WIFSIGNALED(status))
        child_status = WTERMSIG(status);
      else
        child_status = 1;

      int child_sock = it->second.socket;

      write(child_sock, &child_status, sizeof(child_status));
      close(child_sock);

      child_procs.erase(it);
    }
  }
}

void kill_old_procs() {

  time_t now = time(NULL);
  int i = 0;

  for (auto it = child_procs.begin(); it != child_procs.end(); ++it) {
    i++;
    if (difftime(now, it->second.forked_at) > MAX_CHILD_PROC_TIMEOUT) {
      kill(it->first, SIGKILL);
    }
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

  signal(SIGPIPE, SIG_IGN);

  // Create a "self-pipe" for communication between child signal handler and main loop
  if (pipe(sigchld_pipe) == -1) {
    perror("pipe");
    return 1;
  }
  fcntl(sigchld_pipe[0], F_SETFL, O_NONBLOCK);
  // fcntl(sigchld_pipe[1], F_SETFL, O_NONBLOCK);

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
  int max_pending_connections = get_env_var_int("ADAGUC_NUMPARALLELPROCESSES", DEFAULT_QUEUED_CONNECTIONS);
  printf("Max pending connections: %d\n", max_pending_connections);
  if (listen(listen_socket, max_pending_connections) != 0) {
    printf("Error on listen call \n");
    return 1;
  }

  printf("@@@ Entering fork server loop\n");
  time_t last_cleanup = time(NULL);

  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listen_socket, &readfds);
    FD_SET(sigchld_pipe[0], &readfds);

    int maxfd = std::max(listen_socket, sigchld_pipe[0]);

    // Select will block until there is activity on either listen_socket or sigchld_pipe, or until the `timeval tv` has passed.
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int ready = select(maxfd + 1, &readfds, NULL, NULL, &tv);

    if (ready == -1) {
      if (errno == EINTR) continue;
      perror("select");
      continue;
    }

    // Check for dead processes
    time_t now = time(NULL);
    if (now - last_cleanup >= CHECK_CHILD_PROC_INTERVAL) {
        kill_old_procs();
        last_cleanup = now;
    }

    // ️Handle child exit events
    if (FD_ISSET(sigchld_pipe[0], &readfds)) {
      handle_child_exit_events();
    }

    if (FD_ISSET(listen_socket, &readfds)) {
      socklen_t sock_len = sizeof(remote);

      // Once someone connects to the unix socket, immediately fork and execute the client request in `handle_client`
      int client_socket = accept(listen_socket, (struct sockaddr *)&remote, &sock_len);
      if (client_socket == -1) continue;

      pid_t pid = fork();
      // printf("After fork, pid %d\n", pid);
      if (pid == 0) {
        // Child process handles request. Communication with python happens through `client_socket`
        close(listen_socket);
        handle_client(client_socket, run_adaguc_once, argc, argv, envp);
        _exit(1);
      } else if (pid > 0) {
        // Parent process keeps track of new socket and returns to listen for new connections
        child_proc_t child_proc = {client_socket, time(NULL)};
        child_procs[pid] = child_proc;
      } else {
        close(client_socket);
      }
    }
  }

  return 0;
}