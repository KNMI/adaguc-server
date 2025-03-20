#include <map>
#include <mutex>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include "CDebugger.h"
#include "CTString.h"
#include "fork_server.h"

// Check this many seconds for old left-over processes
const int CHECK_CHILD_PROC_INTERVAL = 60;
// Allow a backlog of this many connections. Uses `ADAGUC_NUMPARALLELPROCESSES` env var.
const int DEFAULT_QUEUED_CONNECTIONS = 4;
// Old left-over child process should be killed after this many seconds. Uses `ADAGUC_MAX_PROC_TIMEOUT` env var.
const int DEFAULT_MAX_PROC_TIMEOUT = 300;

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
  if (data_recv > 0) {
    // The child stdout should end up in the client socket
    dup2(client_socket, STDOUT_FILENO);

    setenv("QUERY_STRING", recv_buf, 1);

    int status = run_adaguc_once(argc, argv, envp, true);
    // fprintf(stderr, "exiting, status=%d", status);

    // fflush(stdout);
    // fflush(stderr);

    exit(status);
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

    int index = pid_index[child_pid].load();
    if (index >= 0 && index < MAX_PROCS) {
      // children[index].exited.store(true); // ✅ Mark child as exited
      children[index].pid.store(-1);

      write(children[index].socket, reinterpret_cast<const char *>(&child_status), sizeof(child_status));
      close(children[index].socket);
    }

    // int child_sock = child_procs[child_pid].socket;
    // fprintf(stderr, "@ on_child_exit [%d] [%d] [%d] [%d]\n", child_pid, child_sock, child_signal, child_status);

    // Write the status code from the child pid into the unix socket back to python
    // write(child_sock, reinterpret_cast<const char *>(&child_status), sizeof(child_status));
    // close(child_sock);

    // child_procs.erase(child_pid);
  }
}

void *clean_child_procs(void *arg) {
  /*
  Every `CHECK_CHILD_PROC_INTERVAL` seconds, check all child procs stored in map.
  If child proc was started more than `ADAGUC_MAX_PROC_TIMEOUT` seconds ago, send SIGKILL.
  */

  int max_child_proc_timeout = get_env_var_int("ADAGUC_MAX_PROC_TIMEOUT", DEFAULT_MAX_PROC_TIMEOUT);
  CDBDebug("Checking every %d seconds for processes older than %d seconds\n", CHECK_CHILD_PROC_INTERVAL, max_child_proc_timeout);
  while (1) {
    time_t now = time(NULL);

    // for (const auto &child_proc_mapping : child_procs) {
    //   child_proc_t child_proc = child_proc_mapping.second;

    //   if (difftime(now, child_proc.forked_at) < max_child_proc_timeout) {
    //     continue;
    //   }

    //   // CDBWarning("Child process with pid %d running longer than %d, sending SIGKILL to clean up\n", child_proc_mapping.first, max_child_proc_timeout);
    //   if (kill(child_proc_mapping.first, SIGKILL) == -1) {
    //     // CDBError("Failed to send SIGKILL to child process %d\n", child_proc_mapping.first);
    //     // TODO: What to do if we cannot SIGKILL child process?
    //   }
    // }

    for (int i = 0; i < MAX_PROCS; i++) {
      if (difftime(now, children[i].forked_at) > max_child_proc_timeout) { // ✅ Safe atomic read
        // close(children[i].socket);                   // Clean up socket
        pid_index[children[i].pid.load()].store(-1); // Remove PID lookup
        children[i].pid.store(-1);                   // Mark slot as free
        children[i].exited.store(false);
      }
    }

    sleep(CHECK_CHILD_PROC_INTERVAL);
  }
}

void add_child_proc(pid_t pid, int socket) {
  for (int i = 0; i < MAX_PROCS; i++) {
    int expected = -1;
    if (children[i].pid.compare_exchange_strong(expected, pid)) {
      children[i].socket = socket;
      children[i].forked_at = time(NULL);
      pid_index[pid].store(i); // ✅ Store PID lookup
      return;
    }
  }
}

int run_as_fork_service(int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp) {
  /*
  Start adaguc in fork mode. This means:
  - Set up a signal handler for any child processes
  - Start a thread in the background that cleans up left-over child processes
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

  // Start cleaning thread in the background
  // TODO: this thread results in segfaults :)
  pthread_t clean_child_procs_thread;
  pthread_create(&clean_child_procs_thread, NULL, clean_child_procs, NULL);

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
  if (listen(listen_socket, max_pending_connections) != 0) {
    printf("Error on listen call \n");
    return 1;
  }

  printf("@@@ Entering fork server loop\n");
  while (1) {
    socklen_t sock_len = sizeof(remote);

    // Once someone connects to the unix socket, immediately fork and execute the client request in `handle_client`
    printf("@@@ Before accept\n");
    if ((client_socket = accept(listen_socket, (struct sockaddr *)&remote, &sock_len)) == -1) {
      printf("@@@ Error on accept() call \n");
      return 1;
    }
    printf("@@@ After accept, before fork\n");

    pid_t pid = fork();
    printf("@@@ After fork, pid %d\n", pid);
    if (pid == 0) {
      // Child process handles request. Communication with python happens through `client_socket`
      close(listen_socket);
      handle_client(client_socket, run_adaguc_once, argc, argv, envp);
    } else if (pid > 0) {
      // Parent process keeps track of new socket and returns to listen for new connections
      add_child_proc(pid, client_socket);
      // child_proc_t child_proc = {client_socket, time(NULL)};
      // child_procs[pid] = child_proc;
    } else {
      printf("@@@ Error on fork() call");
      close(client_socket); // Close the socket if fork fails
    }
  }

  return 0;
}