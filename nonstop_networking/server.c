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
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "format.h"
#include "common.h"

// Forward declarations
void shutdown_server();
void handle_sigint();
void delete_server_dir();

// Globals
static int SERVER_SOCKET = -1;
static struct addrinfo * RESULT = NULL;
static int EPOLL_FD = -1;
static int MAX_CLIENTS = 1024; // HACK: Hopefully this is big enough...
static char * SERVER_DIR = NULL;

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

    // Create temporary directory for files
    SERVER_DIR = calloc(1, 9);
    memcpy(SERVER_DIR, "./XXXXXX", 9);
    if (!(SERVER_DIR = mkdtemp(SERVER_DIR))) {
        exit(1);
    }
    
    int max_events = 500; // HACK: Random value, might not work...
    struct epoll_event events[max_events];
    int timeout_ms = 1000;

    // Main server loop
    while (true) {
        int nfds = epoll_wait(EPOLL_FD, events, max_events, timeout_ms);
        if (nfds == -1) {
            shutdown_server();
            exit(1);
        }

        int i;
        for (i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            //int event = events[i].events;

            if (fd == SERVER_SOCKET) {
                // Event was from the server socket, accept new connection
                int connect_fd = accept(SERVER_SOCKET, NULL, NULL);
                fprintf(stderr, "Accepted new connection with fd: %d\n", connect_fd);
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
                // TODO: Figure out how to detect client disconnect

                // Event was from a client
                fprintf(stderr, "Event from client_fd: %d\n", fd);
                char read_buf[1024];
                memset(read_buf, 0, 1024);
                if (read_all(fd, read_buf, 1024) == -1) {
                    fprintf(stderr, "Read failed!\n");
                }
                fprintf(stderr, "Content: %s\n", read_buf);
                shutdown(fd, SHUT_RD); // Only close early in LIST

                // Check to see what command was given
                if (strncmp(read_buf, "LIST", 4) == 0) {
                    fprintf(stderr, "LIST request detected\n");
                    // Handle LIST request
                    char send_buf[1024];
                    memset(send_buf, 0, 1024);
                    memcpy(send_buf, "OK\n", 3);

                    size_t send_size = 0;
                    size_t bytes_copied = 3 + sizeof(size_t);
                    fprintf(stderr, "Copying file names...\n");

                    fprintf(stderr, "Reading file names from directory: %s\n", SERVER_DIR);
                    DIR * d = opendir(SERVER_DIR);
                    struct dirent * dir;
                    if (d) {
                        while ((dir = readdir(d))) {
                            fprintf(stderr, "Found file: %s\n", dir->d_name);
                            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                                continue; // Skip self and parent names
                            }
                            
                            send_size += strlen(dir->d_name);
                            memcpy(send_buf + bytes_copied, dir->d_name, strlen(dir->d_name));
                            bytes_copied += strlen(dir->d_name);
                            send_buf[bytes_copied] = '\n';
                            send_size++;
                            bytes_copied++;
                        }
                        if (send_size > 0) {
                            send_buf[bytes_copied - 1] = '\0'; // Get rid of trailing newline
                            bytes_copied--;
                            send_size--;
                        }

                        closedir(d);
                    }
                    fprintf(stderr, "send size: %zu\n", send_size);
                    memcpy(send_buf + 3, (void *)&send_size, sizeof(size_t));
                    write(2, send_buf, 3);
                    write(2, send_buf + 3 + sizeof(size_t), 255);

                    write_all(fd, send_buf, bytes_copied);
                    fprintf(stderr, "Completed LIST request\n");
                    epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                } else if (strncmp(read_buf, "GET", 3) == 0) {
                    // Handle GET request
                } else if (strncmp(read_buf, "PUT", 3) == 0) {
                    // Handle PUT request
                } else if (strncmp(read_buf, "DELETE", 6) == 0) {
                    // Handle DELETE request
                } else {
                    // Handle Invalid
                    // Shouldn't go here
                    exit(7);
                }
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

    // Remove all server files and delete temp directory
    fprintf(stderr, "Deleting files and directory\n");
    delete_server_dir();
    free(SERVER_DIR);
}

void handle_sigint() {
    shutdown_server();
    exit(0);
}

void delete_server_dir() {
    DIR * d = opendir(SERVER_DIR);
    struct dirent * dir;
    if (d) {
        while ((dir = readdir(d))) {
            size_t path_size = strlen(SERVER_DIR) + 1 + strlen(dir->d_name) + 1;
            char remove_path[path_size];
            memset(remove_path, 0, path_size);
            memcpy(remove_path, SERVER_DIR, strlen(SERVER_DIR));
            remove_path[strlen(SERVER_DIR)] = '/';
            memcpy(remove_path + strlen(SERVER_DIR) + 1, dir->d_name, strlen(dir->d_name));
            remove(remove_path);
        }

        closedir(d);
    }
    remove(SERVER_DIR);
}
