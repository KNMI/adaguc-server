#ifndef ADAGUC_SERVER_FORK_SERVER_H
#define ADAGUC_SERVER_FORK_SERVER_H

#include <map>
#include <time.h>

typedef struct {
  int socket;
  time_t forked_at;
} child_proc_t;

static std::map<pid_t, child_proc_t> child_procs;

void handle_client(int client_socket, int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp);
void child_signal_handler(int child_signal);
int run_as_fork_service(int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp);

#endif // ADAGUC_SERVER_FORK_SERVER_H