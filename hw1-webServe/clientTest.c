#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>

void err_sys(const char *x)
{
    perror(x);
    exit(1);
}

int main(int argc, char **argv)
{

    int sockfd, n;
    char recive[100];
    struct sockaddr_in servaddr;

    if (argc != 2)
        err_sys("para error");

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(80);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_sys("inet error");

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        err_sys("connect error");

    //listen(listenfd, 10);
    write(sockfd, "fuck you\n", 12);

    while ((n = read(sockfd, recive, 100)) > 0)
    {
        recive[n] = 0;

        fputs(recive, stdout);
    }

    if (n < 0)
        err_sys("read error");

    exit(0);
}