# ADAGUC Fork Server Overview

This document describes how the ADAGUC fork server handles requests using a persistent "mother" process that spawns short-lived child processes via `fork()`.
Each request is handled in a separate child process, while the mother process remains initialized and ready to accept new connections.

This design reduces per-request overhead by avoiding repeated process initialization.

The fork server is enabled by setting the environment variable `ADAGUC_FORK_ENABLE` to `TRUE`. If this variable is not set to `TRUE`, ADAGUC runs without the fork server.

# Components

The system consists of three parts:

- Python server: Handles HTTP/WMS requests, forwards them to the fork server, and returns the response to the client.
- Mother process (C++): A persistent process that listens for requests and manages child processes.
- Child processes (C++): Short-lived processes created with `fork()`. Each child handles one request.

The maximum number of concurrent children is limited by the environment variable `ADAGUC_NUMPARALLELPROCESSES`.

Communication between the Python server and the mother process occurs via a Unix domain socket.

# Request Lifecycle

## 0. Python starts

- Python launches the fork server (mother process)

## 1. Server Startup

The mother process performs the following steps:

- Setup signal handlers for `SIGCHLD`.
- Create a self-pipe, so the signal handler can notify the main loop when a child exits.
- Create and bind a Unix socket to communicate with the python server.
- Enter the main loop and start listening for connections using `select()`

## 2. Python Sends Request

The Python server receives a WMS request and forwards request data to the Unix socket in the following format:

```
ADAGUC_LOGFILE=...\n
SCRIPT_NAME=...\n
REQUEST_URI=...\n
QUERY_STRING=...
```

Normally, ADAGUC receives these values via environment variables at process startup. With the fork server, the mother process is already running, so these request-specific variables are sent through the socket instead.

## 3. Connection Accepted

The mother process detects activity on the socket, accepts the connection using accept(), and then creates a new process using `fork()`. `fork()` creates a new child process, while the original mother process keeps running.

After `fork()`, the child process inherits the parent’s state, including open file descriptors, memory, and environment. Memory is shared using copy-on-write (COW), meaning pages are only duplicated if either process modifies them.

From the child process perspective, the process continues from the same point as the parent, so there is almost no startup overhead.

## 4. Mother Bookkeeping

The mother keeps track of running children using a map keyed by PID. For each child it stores:

- The client socket
- The time when the process was forked

This information is later used to return the exit status and to clean up old processes.

## 5. Child Handles Request

The child process:

1. Redirects stdout to the client socket.
2. Parses the request data and sets environment variables.
3. Runs the request handler.
4. Writes the result to stdout (which goes to the socket).
5. Exits with a status code.

Each child handles exactly one request and then exits immediately.

## 6. Child Exit Notification

When the child exits, the kernel sends a `SIGCHLD` signal to the mother process. The signal handler runs and writes a byte to the self-pipe.

The handler does not perform complex work because signal handlers must return quickly and may only use async-signal safe functions.

## 7. Mother Handles Exit Event

The main loop monitors the self-pipe. When data is available:

1. The self-pipe is drained.
2. `waitpid()` is called in a loop with `WNOHANG` to collect exited children.

For each exited child:

- The corresponding client socket is retrieved.
- The exit status is written as the final 4 bytes to the socket.
- The socket is closed.
- The child entry is removed from the bookkeeping map.

## 8. Python Reads Response

Python reads the full response from the Unix socket.

The response format is:

```
[WMS response bytes]
[4 byte status code]
```

All bytes except the final four contain the normal ADAGUC response that is returned to the client. The final four bytes represent the exit status of the child process.

# Process Cleanup

The mother periodically checks if child processes run too long, and will send a `SIGKILL` if a child exceeds the configured timeout.


# Supervision

The fork server (mother process) is managed by a Python supervisor `fork_server_supervisor.py`, which:

- Starts the process during application startup
- Performs periodic health checks via the Unix socket
- Restarts the process if it becomes unresponsive or exits
- Terminates the process during application shutdown