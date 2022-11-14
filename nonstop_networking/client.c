/**
 * nonstop_networking
 * CS 341 - Fall 2022
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);
void write_all(int fd, char * buffer, size_t size);
void read_all(int fd, char * buffer, size_t size);

int main(int argc, char **argv) {
    // Good luck!

    // Parse and validate the arguments
    char ** parsed_args = parse_args(argc, argv);
    verb method = check_args(parsed_args);
    // {host, port, method, remote, local, NULL}
    char * host = parsed_args[0];
    char * port = parsed_args[1];
    char * str_method = parsed_args[2];
    // char * remote_file = parsed_args[3];
    // char * local_file = parsed_args[4];

    // Create connection to the server
    struct addrinfo hints, * result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int getaddr_res = getaddrinfo(host, port, &hints, &result);
    if (getaddr_res != 0) {
        // normally print out error here
        fprintf(stderr, "%s\n", gai_strerror(getaddr_res));// TODO remove
        exit(1);
    }

    int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1) {
        //perror(NULL)
        perror(NULL); // TODO: remove
        free(parsed_args);
        exit(1);
    }

    int connect_result = connect(socket_fd, result->ai_addr, result->ai_addrlen);
    if (connect_result == -1) {
        perror(NULL); // TODO: remove
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
        print_connection_closed();
        free(parsed_args);
        exit(1);
    }


    switch (method) {
        case GET: {
            break;
        }
        case PUT: {
            break;
        }
        case LIST: {
            // structure LIST request
            char * write_buf = NULL;
            asprintf(&write_buf, "%s\n", str_method);

            // write request
            write_all(socket_fd, write_buf, strlen(write_buf));
            free(write_buf);
            shutdown(socket_fd, SHUT_WR);

            // prepare read buffer
            char read_buf[4096];
            memset(read_buf, 0, 4096);
            
            // read from server
            read_all(socket_fd, read_buf, (size_t)4096);

            // write results from server
            // TODO: this is not correct, need to parse size
            char * res = strtok(read_buf, "\n");
            while (res) {
                fprintf(stdout, "%s", res);
                res = strtok(NULL, "\n");
            }

            break;
        }
        case DELETE: {
            break;
        }
        default: {
            shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
            free(parsed_args);
            exit(1);
        }
    }

    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    free(parsed_args);
    exit(0);
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}

// write all the bytes to the description, exit on failure
void write_all(int fd, char * buffer, size_t size) {
    size_t bytes_wrote = 0;
    while (bytes_wrote != size) {
        ssize_t cur_written = write(fd, buffer + bytes_wrote, size - bytes_wrote);
        if (bytes_wrote == 0) {
            return;
        } else if (cur_written > 0) {
            bytes_wrote += (size_t)cur_written;
        } else if (cur_written == -1 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        } else {
            // error while writing
            exit(1);
        }
    }
}

void read_all(int fd, char * buffer, size_t size) {
    size_t bytes_read = 0;
    while (bytes_read < size) {
        ssize_t cur_read = read(fd, buffer + bytes_read, size - bytes_read);
        if (cur_read == 0) {
            return;
        } else if (cur_read > 0) {
            bytes_read += (size_t)cur_read;
        } else if (cur_read == -1 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        } else {
            exit(1);
        }
    }
}
