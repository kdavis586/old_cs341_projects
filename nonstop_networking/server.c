/**
 * nonstop_networking
 * CS 341 - Fall 2022
 */

#include <netdb.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "format.h"

// Forward declarations
void shutdown_server();
void handle_sigint();

// Globals
static int SERVER_SOCKET = -1;
static struct addrinfo * RESULT = NULL;
static int EPOLL_FD = -1;
static int MAX_CLIENTS = 2048; // HACK: Hopefully this is big enough...

// Main function
// TODO: Check to make sure returning proper exit code for each scenario
int main(int argc, char **argv) {
    // Check for valid number of arguments
    if (argc != 2) {
        print_server_usage();
        exit(1); 
    }

    // Set up signal handling
    struct sigaction handler;
    memset(&handler, 0, sizeof(struct sigaction));
    handler.sa_handler = handle_sigint;

    if (sigaction(SIGINT, &handler, NULL) == -1) {
        exit(1);
    }

    char * port = argv[1];

    // Create the server socket
    SERVER_SOCKET = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (SERVER_SOCKET == -1) {
        // Normally show error here
        exit(1);
    }

    // Set the server socket options
    const int enable = 1;
    if (setsockopt(SERVER_SOCKET, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        // Normally show error here
        shutdown_server();
        exit(1);
    }
    if (setsockopt(SERVER_SOCKET, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) == -1) {
        // Normally show error here
        shutdown_server();
        exit(1);
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Set up addrinfo for bind
    int getaddr_result = getaddrinfo(NULL, port, &hints, &RESULT);
    if (getaddr_result != 0) {
        shutdown_server();
        exit(1);
    }

    if (bind(SERVER_SOCKET, RESULT->ai_addr, RESULT->ai_addrlen)) {
        shutdown_server();
        exit(1);
    }

    if (listen(SERVER_SOCKET, MAX_CLIENTS) == -1) {
        shutdown_server();
        exit(1);
    }

    // Setting up epoll (input param is ignored)
    int EPOLL_FD = epoll_create(8675309);
    if (EPOLL_FD == -1) {
        shutdown_server();
        exit(1);
    }

    // Add server socket for listening
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.fd = SERVER_SOCKET;
    if (epoll_ctl(EPOLL_FD, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        shutdown_server();
        exit(1);
    }

    struct epoll_event events[MAX_CLIENTS];
    int max_events = 500; // HACK: Random value, might not work...
    int timeout_ms = 1000;
    while (true) { // TODO: Fix the condition here
        int nfds = epoll_wait(EPOLL_FD, events, max_events, timeout_ms);
        if (nfds == -1) {
            shutdown_server();
            exit(1);
        }

        if (nfds == 0) continue;

        int i;
        for (i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            //int event = events[i].events;

            if (fd == SERVER_SOCKET) {
                // Event was from the server socket, accept new connection
                int connect_fd = accept(SERVER_SOCKET, NULL, NULL);
                if (connect_fd == -1) {
                    exit(1);
                }

                if (fcntl(connect_fd, F_SETFD, fcntl(connect_fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
                    exit(1);
                }
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connect_fd;
                if (epoll_ctl(EPOLL_FD, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
                    shutdown_server();
                    exit(1);
                }

            } else {
                // Event was from a client
            }
        }
    }

    // Shouldn't go here
    shutdown_server();
    exit(1);
}

// Helper functions
void shutdown_server() {
    if (SERVER_SOCKET != -1) {
        shutdown(SERVER_SOCKET, SHUT_RDWR);
        close(SERVER_SOCKET);
    }
    if (EPOLL_FD != -1) {
        close(EPOLL_FD);
    }
    if (RESULT) {
        //freeaddrinfo(RESULT);
        free(RESULT);
        RESULT = NULL;
    }
}

void handle_sigint() {
    shutdown_server();
    exit(0);
}
