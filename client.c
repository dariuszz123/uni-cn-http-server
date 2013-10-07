#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char *argv[])
{
    int sockfd, port, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    FILE *fp;

    if (argc < 5) {
       fprintf(stderr,"usage: %s [method] [host] [port] [path]\n", argv[0]);
       fprintf(stderr,"method - GET or HEAD [GET]\n");
       fprintf(stderr,"host - ip [127.0.0.1]\n");
       fprintf(stderr,"port - port number [9091]\n");
       fprintf(stderr,"path - requested file path [/index.html]\n");
       exit(0);
    }

    if(strcmp(argv[1], "GET") != 0 && strcmp(argv[1], "HEAD") != 0) {
        fprintf(stderr,"ERROR, method not supported!\n");
        exit(0);
    }

    port = atoi(argv[3]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        fprintf(stderr,"ERROR opening socket\n");
        exit(0);
    }

    server = gethostbyname(argv[2]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy(
         (char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length
    );

    serv_addr.sin_port = htons(port);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        fprintf(stderr,"ERROR connecting\n");
        exit(0);
    }
    bzero(buffer,256);

    strcpy(buffer, argv[1]);
    strcat(buffer, " ");
    strcat(buffer, argv[4]);
    strcat(buffer, " ");
    strcat(buffer, "HTTP/1.0");

    n = write(sockfd,buffer,strlen(buffer));

    if (n < 0) {
         fprintf(stderr,"ERROR writing to socket\n");
         exit(0);
    }

    bzero(buffer,256);

    int i = 0;
    char buffer2[256];

    while(read(sockfd,buffer,1)) {
        if (i < 4) {
            if (strcmp(buffer, "\n") != 0) {
                strcat(buffer2, buffer);
            } else {
                i++;
                strcpy(buffer2, "");
            }
        } else {
            printf("%s",buffer);
        }
    }

    close(sockfd);

    return 0;
}