#ifndef ADAGUC_SERVER_FORK_SERVER_H
#define ADAGUC_SERVER_FORK_SERVER_H

#include <map>
#include <time.h>

#define MAX_PROCS 32  // Can handle up to 32 child processes
#define MAX_PID 65536 // Max PID value for fast lookup

typedef struct {
  int socket;
  time_t forked_at;
} child_proc_t;

static std::map<pid_t, child_proc_t> child_procs;

// void add_child_proc(pid_t pid, int socket);
void handle_client(int client_socket, int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp);
void child_signal_handler(int child_signal);
int run_as_fork_service(int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp);

#endif // ADAGUC_SERVER_FORK_SERVER_H