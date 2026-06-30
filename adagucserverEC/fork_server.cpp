#include <cerrno>
#include <csignal>
#include <ctime>
#include <fcntl.h>
#include <map>
#include <mutex>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "CDebugger.h"
#include "CTString.h"
#include "fork_server.h"

/*
See also: doc/fork_server.md
- Methods prefixed with `mother_` only get executed by the fork server
- Methods prefixed with `child_` only get executed by the (forked) children
*/

// Check this many seconds for old left-over processes
const int CHECK_CHILD_PROC_INTERVAL = 30;
// Default for ADAGUC_NUMPARALLELPROCESSES, matches the python default
const int DEFAULT_NUM_PARALLEL_PROCESSES = 4;
// Default for ADAGUC_MAX_PROC_TIMEOUT, matches the python default.
const int DEFAULT_MAX_CHILD_PROC_TIMEOUT = 180;

typedef struct {
  int child_socket_fd;
  time_t forked_at;
} child_proc_t;

static std::map<pid_t, child_proc_t> child_procs;
int self_pipe[2];

int mother_get_env_var_int(const char *env, int default_val) {
  CT::string env_var(getenv(env));
  if (!env_var.isInt()) return default_val;
  int value = env_var.toInt();
  return value > 0 ? value : default_val;
}

/**
 * Set the filedescriptor for the "self-pipe" to non-blocking
 *
 * @param fd The file descriptor
 */
int mother_set_fd_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) return 1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1;
}

/**
 * Parses a newline-separated string of key=value pairs and sets them as environment variables.
 *
 * This function takes a string view containing multiple lines, where each line is expected
 * to be in the format `KEY=VALUE`. For each valid line, the function sets an environment
 * variable using `setenv()` with overwrite enabled.
 *
 * Empty lines or lines without an '=' character are ignored.
 *
 * @param request A string view containing the environment variable definitions,
 *                each on a separate line.
 */
void child_set_env_from_request(const std::string_view &request) {
  size_t start = 0;

  while (start < request.size()) {
    size_t end = request.find('\n', start);
    if (end == std::string::npos) end = request.size();

    std::string_view line(request.data() + start, end - start);

    if (!line.empty()) {
      size_t sep = line.find('=');
      if (sep != std::string_view::npos) {
        std::string key(line.substr(0, sep));
        std::string val(line.substr(sep + 1));
        setenv(key.c_str(), val.c_str(), 1);
      }
    }

    start = end + 1;
  }
}

/**
 * Handles a single client connection on a Unix socket.
 *
 * This function reads data from the provided client socket, interprets it as raw adaguc environment variable definitions
 * (or a special "PING" message), sets the corresponding environment variables, and runs the given `run_adaguc_once` function.
 *
 * The child's stdout is redirected to the client socket so that output is sent directly back to the client.
 *
 * If the message is "PING", a "PONG\n" response is sent and the process exits.
 *
 * @param child_socket_fd The file descriptor of the connected client socket.
 * @param run_adaguc_once Pointer to the function that processes the request.
 * @param argc Argument count for `run_adaguc_once`.
 * @param argv Argument vector for `run_adaguc_once`.
 * @param envp Environment vector for `run_adaguc_once`.
 */
void child_handle_client(int child_socket_fd, int (*run_adaguc_once)(int, char **, char **), int argc, char **argv, char **envp) {
  setLoggerPid();

  std::vector<char> recv_buf(4096);
  int data_recv = recv(child_socket_fd, recv_buf.data(), recv_buf.size(), 0);
  if (data_recv <= 0) {
    _exit(1);
  }

  std::string_view request_view(recv_buf.data(), data_recv);
  if (request_view.starts_with("PING")) {
    std::string_view resp = "PONG\n";
    send(child_socket_fd, resp.data(), resp.size(), 0);
    exit(0);
  }

  // The request will contain environment variables. Parse and set these for the child process
  child_set_env_from_request(request_view);

  // The child stdout should end up in the client socket
  dup2(child_socket_fd, STDOUT_FILENO);

  int status = run_adaguc_once(argc, argv, envp);

  // fflush(stdout);
  // fflush(stderr);

  exit(status);
}

/**
 * Signal handler for SIGCHLD to notify the main loop of child process exits.
 *
 * This function is called asynchronously by the kernel whenever a child process exits (normally or due to a signal).
 * Only async-signal-safe functions may be called; in particular, no C++ standard library or printf calls are allowed.
 *
 * Since signals are not queued by the kernel, multiple child exits occurring during signal handling may be coalesced.
 * To safely notify the main process, this handler writes a single byte to the self-pipe, which the main select loop
 * monitors. The main loop is then responsible for calling `waitpid` and handling the actual child exit events.
 *
 * If the pipe is full, write() can fail with EAGAIN. Ignoring the error is fine because a wakeup is already pending
 * and the main loop reaps all exited children.
 *
 * @param Unused signal number (required by the signal handler signature).
 */
void mother_signal_handler_for_children(int) {
  char buf = 1;
  ssize_t result = write(self_pipe[1], &buf, 1);
  (void)result;
}

/**
 * Handles all exited child processes notified via the self-pipe.
 *
 * This function is intended to be called from the main loop after the self-pipe indicates that one or more child processes
 * have exited. It drains the self-pipe (the actual contents are ignored) and then loops over all exited children using
 * `waitpid(-1, &status, WNOHANG)` to collect their exit status.
 *
 * For each exited child found in the `child_procs` map:
 * - The child’s exit status or terminating signal is determined.
 * - The status is sent to the corresponding client socket.
 * - The client socket is closed.
 *
 * This ensures proper cleanup of child processes and notifies the respective clients of their termination status.
 * The function handles multiple child exits in a single call.
 */
void mother_handle_child_exited_events() {
  // Drain self-pipe
  char dummy[64];
  while (read(self_pipe[0], dummy, sizeof(dummy)) > 0);

  pid_t pid;
  int status;

  // Calling `waitpid` with WNOHANG, will loop over all exited children and return if none are found.
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    auto node = child_procs.extract(pid);
    if (!node.empty()) {
      auto &[child_socket_fd, forked_at] = node.mapped();
      int child_status;

      if (WIFEXITED(status))
        child_status = WEXITSTATUS(status);
      else if (WIFSIGNALED(status))
        child_status = WTERMSIG(status);
      else
        child_status = 1;

      // Write exit status to child socket fd and then close it
      write(child_socket_fd, &child_status, sizeof(child_status));
      close(child_socket_fd);
    }
  }
}

/**
 * Terminates child processes that have exceeded the maximum allowed runtime.
 *
 * Iterates over all entries in `child_procs` and compares the current time against the recorded `forked_at` timestamp for each child.
 * If a child has been running longer than `max_child_proc_timeout`, it is forcefully terminated using `SIGKILL`.
 */
void mother_kill_old_child_procs(int max_child_proc_timeout) {
  time_t now = time(NULL);
  for (auto it = child_procs.begin(); it != child_procs.end(); ++it) {
    if (difftime(now, it->second.forked_at) > max_child_proc_timeout) {
      kill(it->first, SIGKILL);
    }
  }
}

/**
 * Runs a fork server to handle incoming Unix socket requests.
 *
 * Sets up a Unix socket at `$ADAGUC_PATH/adaguc.socket`, installs a SIGCHLD handler, and uses a non-blocking
 * self-pipe to safely notify the main loop of child exits.
 *
 * The server blocks on `select()` for activity on the listen socket or self-pipe. On a new client connection, it forks:
 * - The child calls `child_handle_client` to process the request via `run_adaguc_once` and exits.
 * - The parent tracks child sockets in `child_procs` and continues listening.
 *
 * Periodically, long-running children are killed via `mother_kill_old_child_procs`.
 *
 * @param run_adaguc_once Pointer to the function processing a single request.
 * @param argc Argument count for `run_adaguc_once`.
 * @param argv Argument vector for `run_adaguc_once`.
 * @param envp Environment vector for `run_adaguc_once`.
 */
int mother_run_as_fork_service(int (*run_adaguc_once)(int, char **, char **), int argc, char **argv, char **envp) {
  // Ignore broken pipe signals, i.e. if python is not listening to unix socket, ignore to not let process hang.
  signal(SIGPIPE, SIG_IGN);

  // Create a "self-pipe" for communication between child signal handler and main loop
  if (pipe(self_pipe) == -1) {
    perror("pipe");
    return 1;
  }
  // Make self-pipe non-blocking, so reads can drain it and signal-handler writes never block.
  if (mother_set_fd_nonblocking(self_pipe[0]) != 0 || mother_set_fd_nonblocking(self_pipe[1]) != 0) {
    CDBError("Error setting self-pipe non-blocking");
    return 1;
  }

  // Create a signal handler for all children (all received SIGCHLD signals)
  // Signal mask is empty, meaning no additional signals are blocked while the handler is executed
  struct sigaction sa;
  sa.sa_handler = mother_signal_handler_for_children;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, NULL);

  // Create a listen endpoint for communicating through a unix socket
  int listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == listen_socket) {
    CDBError("Error on socket() call");
    return 1;
  }

  struct sockaddr_un local, remote;
  local.sun_family = AF_UNIX;

  CT::string socket_path(getenv("ADAGUC_PATH"));
  socket_path.concat("/adaguc.socket");

  strncpy(local.sun_path, socket_path.c_str(), sizeof(local.sun_path));
  local.sun_path[sizeof(local.sun_path) - 1] = '\0';

  // Remove old adaguc.socket file
  unlink(local.sun_path);

  // Bind name to the local socket, this will create an entry in the filesystem
  int len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
  if (bind(listen_socket, (struct sockaddr *)&local, len) != 0) {
    CDBError("Error on binding socket");
    return 1;
  }

  // Keep one extra child slot above the request limit so ping messages can still be handled when Python's request semaphore is full.
  int max_child_procs = mother_get_env_var_int("ADAGUC_NUMPARALLELPROCESSES", DEFAULT_NUM_PARALLEL_PROCESSES) + 1;
  int max_child_proc_timeout = mother_get_env_var_int("ADAGUC_MAX_PROC_TIMEOUT", DEFAULT_MAX_CHILD_PROC_TIMEOUT);
  CDBDebug("Max child processes: %d", max_child_procs);
  CDBDebug("Max child process timeout: %d", max_child_proc_timeout);

  // Start listening on the socket. Can only accept `max_child_procs` number of children.
  if (listen(listen_socket, max_child_procs) != 0) {
    CDBError("Error on listen call");
    return 1;
  }

  CDBDebug("Entering fork server loop");
  time_t last_cleanup = time(NULL);

  while (1) {
    // fd_set is modified by select(); reinitialize it each loop to monitor listen_socket and self_pipe again
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(self_pipe[0], &readfds);

    // Only wake on select if forkserver is able to handle new connections.
    bool can_accept_child = child_procs.size() < static_cast<size_t>(max_child_procs);
    if (can_accept_child) {
      FD_SET(listen_socket, &readfds);
    }

    int maxfd = std::max(listen_socket, self_pipe[0]);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    // Select will block until there is activity on either listen_socket or self_pipe, or until the `timeval tv` has passed.
    int ready = select(maxfd + 1, &readfds, NULL, NULL, &tv);

    if (ready == -1) {
      if (errno == EINTR) continue;
      perror("select");
      continue;
    }

    // Check for dead processes
    time_t now = time(NULL);
    if (now - last_cleanup >= CHECK_CHILD_PROC_INTERVAL) {
      mother_kill_old_child_procs(max_child_proc_timeout);
      last_cleanup = now;
    }

    // Only run if there is activity on the self_pipe (to handle exit events)
    if (FD_ISSET(self_pipe[0], &readfds)) {
      mother_handle_child_exited_events();
    }

    // Only run if there is activity on the listen_socket (to handle the WMS requests)
    if (can_accept_child && FD_ISSET(listen_socket, &readfds)) {
      socklen_t sock_len = sizeof(remote);

      // Once someone connects to the unix socket, immediately fork and execute the client request in `child_handle_client`
      int child_socket_fd = accept(listen_socket, (struct sockaddr *)&remote, &sock_len);
      if (child_socket_fd == -1) continue;

      pid_t pid = fork();

      // `fork` returns 0 for the child process, and returns the child pid to the mother process
      // Both if/else paths are taken. Mother process takes pid > 0, child process takes pid == 0.
      if (pid == 0) {
        // Child process handles request. Communication with python happens through `child_socket_fd`
        close(listen_socket);
        child_handle_client(child_socket_fd, run_adaguc_once, argc, argv, envp);
        _exit(1);
      } else if (pid > 0) {
        // Parent process keeps track of new socket and returns to listen for new connections
        child_proc_t child_proc = {child_socket_fd, time(NULL)};
        child_procs[pid] = child_proc;
      } else {
        close(child_socket_fd);
      }
    }
  }

  return 0;
}
