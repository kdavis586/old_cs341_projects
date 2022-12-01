/**parse_args
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
#include <sys/stat.h>
#include <fcntl.h>

char **parse_args(int argc, char **argv);
verb check_args(char **args);
int status_err(char * status);

int main(int argc, char **argv) {
    // Good luck!

    // Parse and validate the arguments
    char ** parsed_args = parse_args(argc, argv);
    verb method = check_args(parsed_args);

    char * host = parsed_args[0];
    char * port = parsed_args[1];
    char * str_method = parsed_args[2];
    char * remote_file = parsed_args[3];
    char * local_file = parsed_args[4];

    // Create connection to the server
    struct addrinfo hints, * result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int getaddr_res = getaddrinfo(host, port, &hints, &result);
    if (getaddr_res != 0) {
        // normally print out error here
        //fprintf(stderr, "%s\n", gai_strerror(getaddr_res));
        freeaddrinfo(result);
        exit(1);
    }

    int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd == -1) {
        // normally print out error here
        //perror(NULL);
        free(parsed_args);
        freeaddrinfo(result);
        exit(1);
    }

    int connect_result = connect(socket_fd, result->ai_addr, result->ai_addrlen);
    if (connect_result == -1) {
        // normally print out error here
        //perror(NULL);
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
        print_connection_closed();
        free(parsed_args);
        freeaddrinfo(result);
        exit(1);
    }


    switch (method) {
        case GET: {
            // Prepare request
            char * write_buf = NULL;
            asprintf(&write_buf, "%s %s\n", str_method, remote_file);

            // Send request
            if (write_all(socket_fd, write_buf, strlen(write_buf)) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }
            shutdown(socket_fd, SHUT_WR);
            free(write_buf);

            // Prepare read buffer
            char read_buf[1024];
            memset(read_buf, 0, 1024);
            
            // Read from server
            int bytes_read = -1;
            if ((bytes_read = read_all(socket_fd, read_buf, (size_t)1024)) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Get response status
            char status[256];
            memset(status, 0, 256);
            sscanf(read_buf, "%s\n", status);

            if (status_err(status)) {
                fprintf(stdout, "%s", read_buf);
                break;
            }

            // Get response size
            size_t data_size = 0;
            memcpy((char *)&data_size, read_buf + strlen(status) + 1, sizeof(size_t));

            // Adjust bytes read for only file content
            bytes_read -= (strlen(status) + 1 + sizeof(size_t));
            size_t offset = (strlen(status) + 1 + sizeof(size_t));

            // Read all file data and write results from server local file name
            int out_fd = open(local_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
            if (out_fd == -1) {
                // normally print error here
                //perror(NULL);
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }
            
            size_t recieved_size = 0;
            do {
                if (write_all(out_fd, read_buf + offset, bytes_read) == -1) {
                    shutdown(socket_fd, SHUT_RDWR);
                    close(socket_fd);
                    print_connection_closed();
                    free(parsed_args);
                    freeaddrinfo(result);
                    exit(1);
                }
                recieved_size += bytes_read;
                memset(read_buf, 0, 1024);
                offset = 0;
            } while ((bytes_read = read_all(socket_fd, read_buf, 1024)) != 0);
            if (bytes_read == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Get true response size
            if (recieved_size < data_size) {
                print_too_little_data();
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            } else if (recieved_size > data_size) {
                print_received_too_much_data();
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Write response status and size to stdout
            close(out_fd);
            fprintf(stdout, "%s\n%zu", status, data_size);
            break;
        }
        case PUT: {
            // Check to make sure that local file to upload exists
            struct stat file_info;
            if (stat(local_file, &file_info) != 0) {
                // normally print error here
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Get the size of the file for header information
            size_t file_size = (size_t)file_info.st_size;

            // Create and send the request information
            //    Send the request verb and remote file name
            char * write_buf = NULL;
            asprintf(&write_buf, "%s %s\n", str_method, remote_file);
            if (write_all(socket_fd, write_buf, strlen(write_buf)) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }
            free(write_buf);

            //    Send the local file size
            if (write_all(socket_fd, (char *)&file_size, sizeof(size_t)) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            //    Send the local file contents
            FILE * local_f_ptr = fopen(local_file, "r");
            char * line_buf = NULL;
            size_t len = 0;
            ssize_t bytes_got;
            while((bytes_got = getline(&line_buf, &len, local_f_ptr)) != -1) {
                if (write_all(socket_fd, line_buf, bytes_got) == -1) {
                    free(line_buf);
                    shutdown(socket_fd, SHUT_RDWR);
                    close(socket_fd);
                    print_connection_closed();
                    free(parsed_args);
                    freeaddrinfo(result);
                    exit(1);
                }
                free(line_buf);
            }
            fclose(local_f_ptr);
            shutdown(socket_fd, SHUT_WR);

            // Prepare read buffer
            char read_buf[1024];
            memset(read_buf, 0, 1024);
            
            // Read from server
            if (read_all(socket_fd, read_buf, (size_t)1024) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // // Get response status
            // char status[256];
            // memset(status, 0, 256);
            // sscanf(read_buf, "%s\n", status);

            // if (status_err(status)) {
            //     fprintf(stdout, "%s", read_buf);
            //     break;
            // }

            // Put response will only respond with with either OK\n or ERROR\n<Error info>\n ... just print it
            fprintf(stdout, "%s", read_buf);
            break;
        }
        case LIST: {
            // Create LIST request
            char * write_buf = NULL;
            asprintf(&write_buf, "%s\n", str_method);

            // Send request
            if (write_all(socket_fd, write_buf, strlen(write_buf)) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }
            free(write_buf);
            shutdown(socket_fd, SHUT_WR);

            // Prepare read buffer
            char read_buf[1024];
            memset(read_buf, 0, 1024);
            
            // Read from server
            if (read_all(socket_fd, read_buf, (size_t)1024) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Get response status
            char status[256];
            memset(status, 0, 256);
            sscanf(read_buf, "%s\n", status);

            if (status_err(status)) {
                fprintf(stdout, "%s", read_buf);
                break;
            }

            // Get response size
            size_t data_size = 0;
            memcpy((char *)&data_size, read_buf + strlen(status) + 1, sizeof(size_t));

            // Get true response size
            size_t content_size = strlen(read_buf + strlen(status) + 1 + sizeof(size_t));
            if (content_size < data_size) {
                print_too_little_data();
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            } else if (content_size > data_size) {
                print_received_too_much_data();
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Write results from server to stdout
            fprintf(stdout, "%s\n%s", status, read_buf + strlen(status) + 1 + sizeof(size_t));
            break;
        }
        case DELETE: {
            // Create DELETE request
            char * write_buf = NULL;
            asprintf(&write_buf, "%s %s\n", str_method, remote_file);

            // Send request
            if (write_all(socket_fd, write_buf, strlen(write_buf)) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }
            free(write_buf);
            shutdown(socket_fd, SHUT_WR);

            // Prepare read buffer
            char read_buf[1024];
            memset(read_buf, 0, 1024);
            
            // Read from server
            if (read_all(socket_fd, read_buf, (size_t)1024) == -1) {
                shutdown(socket_fd, SHUT_RDWR);
                close(socket_fd);
                print_connection_closed();
                free(parsed_args);
                freeaddrinfo(result);
                exit(1);
            }

            // Get response status
            char status[256];
            memset(status, 0, 256);
            sscanf(read_buf, "%s\n", status);

            // Delete response will only respond with with either OK\n or ERROR\n<Error info>\n ... just print it
            fprintf(stdout, "%s", read_buf);
            break;
        }
        default: {
            shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
            free(parsed_args);
            freeaddrinfo(result);
            exit(1);
        }
    }

    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    free(parsed_args);
    freeaddrinfo(result);
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

int status_err(char * status) {
    if (strncmp(status, "ERROR", 5) == 0) {
        return 1;
    }

    return 0;
}
