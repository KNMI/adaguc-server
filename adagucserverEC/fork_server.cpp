#include <map>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CTString.h"
#include "fork_server.h"

static std::map<pid_t, int> child_sockets;
static const unsigned int max_pending_connections = 5;

void handle_client(int client_socket, int (*do_work)(int, char **, char **), int argc, char **argv, char **envp) {
  int recv_buf_len = 4096;
  char recv_buf[recv_buf_len];
  memset(recv_buf, 0, recv_buf_len * sizeof(char));

  int data_recv = recv(client_socket, recv_buf, recv_buf_len, 0);
  if (data_recv > 0) {
    // The child stdout should end up in the client socket
    dup2(client_socket, STDOUT_FILENO);

    setenv("QUERY_STRING", recv_buf, 1);

    int status = do_work(argc, argv, envp);
    // fprintf(stderr, "exiting, status=%d", status);

    // fflush(stdout);
    // fflush(stderr);

    exit(0);
  }
}

void on_child_exit(int child_signal) {
  /*
  This function gets executed once a child exits/signals (through SIGCHLD)
  Note: the kernel does not queue signals. If a child exits during the handling of another signal, the exit/signal gets dropped.
  */

  int stat_val;
  pid_t child_pid;

  // Loop over all exited children. WNOHANG makes the call non-blocking.
  while ((child_pid = waitpid(-1, &stat_val, WNOHANG)) > 0) {
    // If child exited normally
    if (WIFEXITED(stat_val)) {
      int child_status = WEXITSTATUS(stat_val);

      int child_sock = child_sockets[child_pid];
      fprintf(stderr, "@ on_child_exit [%d] [%d] [%d] [%d]\n", child_pid, child_sock, child_signal, child_status);

      // Write the status code from the child pid into the unix socket back to python
      write(child_sock, reinterpret_cast<const char *>(&child_status), sizeof(child_status));
      close(child_sock);

      child_sockets.erase(child_pid);
    } else if (WIFSIGNALED(stat_val)) {
      int child_signal2 = WTERMSIG(stat_val);
      fprintf(stderr, "@ on_child_signal [%d] [%d]\n", child_pid, child_signal2);
    }
  }
}

int run_server(int (*do_work)(int, char **, char **), int argc, char **argv, char **envp) {
  int client_socket = 0;

  signal(SIGCHLD, on_child_exit);

  struct sockaddr_un local, remote;
  int len = 0;

  int listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == listen_socket) {
    printf("Error on socket() call \n");
    return 1;
  }

  local.sun_family = AF_UNIX;

  CT::string socket_path(getenv("ADAGUC_PATH"));
  socket_path.concat("/adaguc.socket");

  strncpy(local.sun_path, socket_path.c_str(), sizeof(local.sun_path));
  local.sun_path[sizeof(local.sun_path) - 1] = '\0';

  // Remove old adaguc.socket file
  unlink(local.sun_path);

  // Bind name to the "local" socket
  len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
  if (bind(listen_socket, (struct sockaddr *)&local, len) != 0) {
    printf("Error on binding socket \n");
    return 1;
  }

  if (listen(listen_socket, max_pending_connections) != 0) {
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
      close(listen_socket);
      handle_client(client_socket, do_work, argc, argv, envp);
    } else {
      child_sockets[pid] = client_socket;
    }
  }

  return 0;
}