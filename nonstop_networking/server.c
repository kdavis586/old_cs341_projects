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
#include "includes/vector.h"

// Forward declarations
void shutdown_server();
void handle_sigint();
void delete_server_dir();
void handle_list(int fd);
void handle_get(int fd, char * read_buf);
void handle_put(int fd, char * read_buf, size_t initial_bytes_read);
void handle_delete(int fd, char * read_buf);

// Globals
static int SERVER_SOCKET = -1;
static struct addrinfo * RESULT = NULL;
static int EPOLL_FD = -1;
static int MAX_CLIENTS = 1024; // HACK: Hopefully this is big enough...
static char * SERVER_DIR = NULL;
static vector * SERVER_FILES = NULL;

// Main function
// TODO: Check to make sure returning proper exit code for each scenario
// TODO: Implement this: http://cs341.cs.illinois.edu/assignments/networking_mp#:~:text=Another%20thing%20to,any%20associated%20state.
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

    signal(SIGPIPE, SIG_IGN);
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
    SERVER_DIR = calloc(1, 7);
    memcpy(SERVER_DIR, "XXXXXX", 6);
    if (!(SERVER_DIR = mkdtemp(SERVER_DIR))) {
        exit(1);
    }
    print_temp_directory(SERVER_DIR);

    // Create vector that stores the names of all files on the server
    SERVER_FILES = string_vector_create();
    
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
                // Event from client, create read_buffer
                char read_buf[1024];
                memset(read_buf, 0, 1024);

                size_t bytes_read = 0;
                if ((bytes_read = read_all(fd, read_buf, 1024)) == -1) {
                    exit(1);
                }

                // Check to see what command was given
                if (strncmp(read_buf, "LIST", 4) == 0) {
                    handle_list(fd);
                    epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                } else if (strncmp(read_buf, "GET", 3) == 0) {
                    // Handle GET request
                    handle_get(fd, read_buf);
                    epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                } else if (strncmp(read_buf, "PUT", 3) == 0) {
                    // Handle PUT request
                    handle_put(fd, read_buf, bytes_read); // TODO: What if need to continue?
                    epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                } else if (strncmp(read_buf, "DELETE", 6) == 0) {
                    // Handle DELETE request
                    handle_delete(fd, read_buf);
                    epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                } else {
                    // Handle Invalid
                    // Send bad response to client
                    // See: http://cs341.cs.illinois.edu/assignments/networking_mp#:~:text=filenames%20at%20all.-,Error%20handling,-Your%20server%20is
                    char * err_string = "ERROR\nBad request\n";
                    write_all(fd, err_string, strlen(err_string) + 1);
                    epoll_ctl(EPOLL_FD, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
            }
        }
    }

    // Shouldn't go here
    shutdown_server();
    exit(1);
}

// Helper functions
void handle_list(int fd) {
    shutdown(fd, SHUT_RD); // Only close early in LIST
                    
    // Create response buffer
    char res_buf[1024];
    memset(res_buf, 0, 1024);
    memcpy(res_buf, "OK\n", 3);

    size_t send_size = 0; // Used to determine message size
    size_t bytes_copied = 3 + sizeof(size_t);

    DIR * d = opendir(SERVER_DIR);
    struct dirent * dir;
    if (d) {
        while ((dir = readdir(d))) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue; // Skip self and parent directories
            }
            
            send_size += strlen(dir->d_name);
            memcpy(res_buf + bytes_copied, dir->d_name, strlen(dir->d_name));
            bytes_copied += strlen(dir->d_name);
            res_buf[bytes_copied] = '\n';
            send_size++;
            bytes_copied++;
        }
        if (send_size > 0) {
            // Get rid of trailing newline and adjust counters
            res_buf[bytes_copied - 1] = '\0';
            bytes_copied--;
            send_size--;
        }

        closedir(d);
    }

    // Copy in message size to the result buffer
    memcpy(res_buf + 3, (void *)&send_size, sizeof(size_t));

    write_all(fd, res_buf, bytes_copied);
}

void handle_get(int fd, char * read_buf) {
    shutdown(fd, SHUT_RD);
    strtok(read_buf, " "); // Split space and rid command
    char * file_name = strtok(NULL, "\n"); // Get filepath of command

    // Check to make sure that filepath exists

    //      Create the full file path
    size_t path_len = strlen(SERVER_DIR) + 1 + strlen(file_name) + 1;
    char full_file_path[path_len];
    memset(full_file_path, 0, path_len);
    memcpy(full_file_path, SERVER_DIR, strlen(SERVER_DIR));
    full_file_path[strlen(SERVER_DIR)] = '/';
    memcpy(full_file_path + strlen(SERVER_DIR) + 1, file_name, strlen(file_name));

    // Create response buffer
    char res_buf[1024];
    memset(res_buf, 0, 1024);

    struct stat file_info;
    bool in_file_list = false;
    size_t i;

    // Check to see if the file names in the server file list
    for (i = 0; i < vector_size(SERVER_FILES); i++) {
        char * fname = vector_get(SERVER_FILES, i);
        if (strcmp(fname, file_name) == 0) {
            in_file_list = true;
            break;
        }
    }
    if (stat(full_file_path, &file_info) == -1 || !in_file_list) {
        // File did not exist in the temporary directory, send bad response
        char * err_string = "ERROR\nNo such file\n";
        memcpy(res_buf, err_string, strlen(err_string));
        write_all(fd, res_buf, strlen(err_string) + 1);
    } else {
        // Send good response
        memcpy(res_buf, "OK\n", 3);
        size_t bytes_to_send = file_info.st_size;
        memcpy(res_buf + 3, (void *)&bytes_to_send, sizeof(size_t));
        
        // Send response info
        write_all(fd, res_buf, 3 + sizeof(size_t)); 

        // Send file data
        int send_fd = open(full_file_path, O_RDONLY);
        if (send_fd == -1) {
            exit(1);
        }
        memset(res_buf, 0, 1024);
        size_t bytes_written = 0;
        while (bytes_written < bytes_to_send) {
            size_t write_size = 1024;
            if (bytes_to_send - bytes_written < 1024) {
                write_size = bytes_to_send - bytes_written;
            }
            if (read_all(send_fd, res_buf, write_size) == -1) {
                exit(1);
            }
            int bytes_wrote = write_all(fd, res_buf, write_size);
            if (bytes_wrote == -1) {
                exit(1);
            }
            bytes_written += (size_t)bytes_wrote;
            memset(res_buf, 0, 1024);
        }
    }

    return;
}

void handle_put(int fd, char * read_buf, size_t initial_bytes_read) {
    //
    size_t i = 0;
    while (read_buf[i] != '\n') {
        i++;
    }
    size_t client_path_size = i - 4;
    char client_path[client_path_size + 1];
    memset(client_path, 0, client_path_size + 1);
    memcpy(client_path, read_buf + 4, client_path_size);

    // Create file to upload client data to

    //      Create the full file path
    size_t path_len = strlen(SERVER_DIR) + 1 + strlen(client_path) + 1;
    char full_file_path[path_len];
    memset(full_file_path, 0, path_len);
    memcpy(full_file_path, SERVER_DIR, strlen(SERVER_DIR));
    full_file_path[strlen(SERVER_DIR)] = '/';
    memcpy(full_file_path + strlen(SERVER_DIR) + 1, client_path, strlen(client_path));

    //      Create the file in the server dir
    int file_fd = open(full_file_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file_fd == -1) {
        exit(1);
    }
    
    // Get the intended size of the client file
    size_t intended_size = 0;
    memcpy((void*)&intended_size, read_buf + 4 + client_path_size + 1, sizeof(size_t));

   //char * data_start = read_buf + 4 + client_path_size - 1 + 1 + sizeof(size_t);
    size_t bytes_to_write = initial_bytes_read - (4 + client_path_size + 1 + sizeof(size_t));
    size_t offset = 4 + client_path_size + 1 + sizeof(size_t);
    int bytes_wrote = 0;
    do {
        bytes_wrote += write_all(file_fd, read_buf + offset, bytes_to_write);
        offset = 0;
        bytes_to_write = read_all(fd, read_buf, 1024);
    } while (bytes_to_write != 0);

    shutdown(fd, SHUT_RD);

    // Create response buffer
    char res_buf[1024];
    memset(res_buf, 0, 1024);

    if ((size_t)bytes_wrote == intended_size) {
        // Good response
        vector_push_back(SERVER_FILES, client_path);
        memcpy(res_buf, "OK\n", 3);
        
        // Send response info
        write_all(fd, res_buf, 3); 
    } else {
        // Handle too much / too little data
        // Delete local file
        unlink(full_file_path);
        char * err_string = "ERROR\nBad file size\n";
        memcpy(res_buf, err_string, strlen(err_string));

        // Send response info
        write_all(fd, res_buf, strlen(err_string) + 1);
    }

    // struct stat file_info;
    // if (stat(full_file_path, &file_info) == -1) {
    //     // File did not exist in the temporary directory, send bad response
    //     char * err_string = "ERROR\nNo such file\n";
    //     memcpy(res_buf, err_string, strlen(err_string));
    //     write_all(fd, res_buf, strlen(err_string) + 1);
    // } else {
    //     // Send good response
        

    //     // Send file data
    //     int send_fd = open(full_file_path, O_RDONLY);
    //     if (send_fd == -1) {
    //         exit(1);
    //     }
    //     memset(res_buf, 0, 1024);
    //     size_t bytes_written = 0;
    //     while (bytes_written < bytes_to_send) {
    //         size_t write_size = 1024;
    //         if (bytes_to_send - bytes_written < 1024) {
    //             write_size = bytes_to_send - bytes_written;
    //         }
    //         if (read_all(send_fd, res_buf, write_size) == -1) {
    //             exit(1);
    //         }
    //         int bytes_wrote = write_all(fd, res_buf, write_size);
    //         if (bytes_wrote == -1) {
    //             exit(1);
    //         }
    //         bytes_written += (size_t)bytes_wrote;
    //         memset(res_buf, 0, 1024);
    //     }
    // }
    return;
}

void handle_delete(int fd, char * read_buf) {
    shutdown(fd, SHUT_RD);
    strtok(read_buf, " "); // Split space and rid command
    char * file_name = strtok(NULL, "\n"); // Get filepath of command

    // Check to make sure that filepath exists

    //      Create the full file path
    size_t path_len = strlen(SERVER_DIR) + 1 + strlen(file_name) + 1;
    char full_file_path[path_len];
    memset(full_file_path, 0, path_len);
    memcpy(full_file_path, SERVER_DIR, strlen(SERVER_DIR));
    full_file_path[strlen(SERVER_DIR)] = '/';
    memcpy(full_file_path + strlen(SERVER_DIR) + 1, file_name, strlen(file_name));

    // Create response buffer
    char res_buf[1024];
    memset(res_buf, 0, 1024);
    size_t data_size = 0;

    struct stat file_info;
    bool in_file_list = false;
    size_t i;

    // Check to see if the file names in the server file list
    for (i = 0; i < vector_size(SERVER_FILES); i++) {
        char * fname = vector_get(SERVER_FILES, i);
        if (strcmp(fname, file_name) == 0) {
            in_file_list = true;
            break;
        }
    }
    if (stat(full_file_path, &file_info) == -1 || !in_file_list) {
        // File did not exist in the temporary directory, send bad response
        char * err_string = "ERROR\nNo such file\n";
        memcpy(res_buf, err_string, strlen(err_string));
        data_size += (strlen(err_string) + 1);

    } else {
        // Send good response
        unlink(full_file_path);
        memcpy(res_buf, "OK\n", 3);
        data_size += 3;
    }

    write_all(fd, res_buf, data_size);
}

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
    delete_server_dir();
    free(SERVER_DIR);

    vector_destroy(SERVER_FILES);
}

void handle_sigint() {
    shutdown_server();
    exit(0);
}

void delete_server_dir() {
    DIR * d = opendir(SERVER_DIR);
    if (d) {
        size_t i;
        for (i = 0; i < vector_size(SERVER_FILES); i++) {
            char * file_name = vector_get(SERVER_FILES, i);
            size_t path_size = strlen(SERVER_DIR) + 1 + strlen(file_name) + 1;
            char remove_path[path_size];
            memset(remove_path, 0, path_size);
            memcpy(remove_path, SERVER_DIR, strlen(SERVER_DIR));
            remove_path[strlen(SERVER_DIR)] = '/';
            memcpy(remove_path + strlen(SERVER_DIR) + 1, file_name, strlen(file_name));
            unlink(remove_path);
        }

        closedir(d);
    }
    rmdir(SERVER_DIR);
}
