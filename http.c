#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdbool.h>

// Get file size in bytes
int get_file_size(char *filename) {
    int size;
    FILE *fp;

    fp = fopen(filename, "r");

    fseek(fp, 0, SEEK_END); // seek to end of file
    size = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file

    fclose(fp);

    return size;
}

// send file to client
void send_file(FILE *client, char *filename) {
    FILE *fp;
    int *buf;

    fp = fopen(filename, "r");

    while(!feof(fp)) {
        fread(buf,1,1,fp);
        fwrite(buf,1,1,client);
    }

    fclose(fp);
}

// guess content type
char *get_content_type(char *filename) {

    char *ext = strrchr(filename, '.');

    if (!ext) {
        return "text/plain";
    }

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html";
    }

    if (strcmp(ext, ".txt") == 0) {
        return "text/plain";
    }

    if (strcmp(ext, ".css") == 0) {
            return "text/css";
    }

    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    }

    if (strcmp(ext, ".gif") == 0) {
        return "image/gif";
    }

    if (strcmp(ext, ".png") == 0) {
        return "image/png";
    }

    return "text/plain";
}

// send basic headers
void send_headers(
    FILE *sock_fd,
    int status_code,
    char *status_phrase,
    char *content_type,
    int content_length
) {
    fprintf(sock_fd, "HTTP/1.0 %d %s\r\n", status_code, status_phrase);
    fprintf(sock_fd, "Content-Type: %s\r\n", content_type);
    fprintf(sock_fd, "Content-Length: %d\r\n", content_length);
    fprintf(sock_fd, "\r\n");
}

// check if file exists
int file_exists(char *filename) {
    FILE *file;
    file = fopen(filename, "r");

    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// process client request
void proccess_request(int fd) {
    FILE *client;
    char filename[256];
    char buffer[256];

    char *line;
    char *method;
    char *path;
    char *version;

    read(fd, buffer, 255);

    line = strtok(buffer, "\n");

    method = strtok(line, " ");
    path = strtok(NULL, " ");
    version = strtok(NULL, " ");

    client = fdopen(fd, "r+");

    if ((strcmp(method, "GET") == 0) || (strcmp(method, "HEAD") == 0)) {

        strcpy(filename, "public");

        fprintf(stderr, "Requested: %s\n", path);

        if(strcmp(path, "/") == 0) {
            strcpy(filename, "public/index.html");
        } else {
            strcat(filename, path);
        }

        if(file_exists(filename) == 1) {
            send_headers(client, 200, "OK", get_content_type(filename), get_file_size(filename));

            if (strcmp(method, "GET") == 0) {
                send_file(client, filename);
                fprintf(stderr, "File sent [%s]\n", filename);
            }
        } else{
            send_headers(client, 404, "Not Found", "text/plain", 10);
            fprintf(client, "Not found!");
            fprintf(stderr, "Requested file not found [%s]\n", filename);
        }

    } else {
        send_headers(client, 500, "Unknown Request Method", "text/plain", 24);
        fprintf(client, "Unknown Request Method!");
    }

    fflush(client);
}

int main(int argc, char *argv[])
{
    int sock_fd;
    int new_sock_fd;
    FILE *client;
    int port;
    int cli_len;
    int pid;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    char buffer[256];

    // no port error
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided. usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // create new socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // socket create error
    if (sock_fd < 0) {
        fprintf(stderr, "ERROR, opening port\n");
        exit(1);
    }

    // set all values in buffer to zero
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // convert string to int
    port = atoi(argv[1]);

    // code for the address family. It should always be set to the symbolic constant AF_INET.
    serv_addr.sin_family = AF_INET;

    // htons() converts a port number in host byte order to a port number in network byte order
    serv_addr.sin_port = htons(port);

    // For server code, this will always be the IP address of the machine on which the server is running, and there is a symbolic constant INADDR_ANY which gets this address.
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // system call binds a socket to an address
    if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "ERROR, unable to bind port\n");
        exit(1);
    }

    // system call allows the process to listen on the socket for connections
    listen(sock_fd, 1);

    cli_len = sizeof(cli_addr);

    while (1) {
        // system call causes the process to block until a client connects to the server
        new_sock_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &cli_len);
        if (new_sock_fd < 0) {
           fprintf(stderr, "ERROR, accepting client request\n");
           exit(1);
        }

        // create child process
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ERROR, cant fork\n");
            exit(1);
        }

        // child process
        if (pid == 0) {
            proccess_request(new_sock_fd);
        }

        close(new_sock_fd);
    }

    close(sock_fd);

    return 0;
}