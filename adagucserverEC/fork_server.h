#ifndef ADAGUC_SERVER_FORK_SERVER_H
#define ADAGUC_SERVER_FORK_SERVER_H

void handle_client(int client_socket, int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp);
void on_child_exit(int child_signal);
int run_as_fork_service(int (*run_adaguc_once)(int, char **, char **, bool), int argc, char **argv, char **envp);

#endif // ADAGUC_SERVER_FORK_SERVER_H