#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <netdb.h>

#define MAX_CONNECT_NUM 10
#define BUFF_SIZE 500000
int listenfd;

void err_sys(const char *x)
{
    perror(x);
    exit(1);
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
    return;
}

void sig_int(int signo)
{
    printf("\nClosing serve socket...\n");
    close(listenfd);

    exit(0);
}

char *get_file(char *buff)
{
    char *start = buff;
    start = strstr(start, "\r\n\r\n");
    start = start + 5;
    start = strstr(start, "\r\n\r\n");
    start = start + 4;

    return start;
}

int get_filelen(char *buff)
{
    char *start = buff;
    start = strstr(start, "Content-Length: ");
    start = strstr(start, " ");

    char *end = strstr(start, "\r\n");
    *end = '\0';

    return atoi(start);
}

char *get_filetype(char *buff)
{
    char *start = buff;
    start = strstr(start, "Content-Type: ");
    start = start + 1;
    start = strstr(start, "Content-Type: ");
    start = strstr(start, " ");
    start = start + 1;

    char *end = strstr(start, "\r\n");
    *end = '\0';

    return start;
}

void save_file(char *content, int length, char *type)
{
    if (strncmp("image/png", type, 9) == 0)
    {
        printf("Saved file type:%s\n", type);
        printf("Saved file length:%d\n", length);

        FILE *fp = fopen("page/img1.png", "w+");
        fseek(fp, 0, SEEK_SET);
        fwrite(content, sizeof(char), length, fp);
        fclose(fp);
    }
    else if (strncmp("image/jpg", type, 9) == 0)
    {
        printf("Saved file type:%s\n", type);
        printf("Saved file length:%d\n", length);

        FILE *fp = fopen("page/jpg1.jpg", "w+");
        fseek(fp, 0, SEEK_SET);
        fwrite(content, sizeof(char), length, fp);
        fclose(fp);
    }
}

void response_req(int connfd, char *path)
{
    if (strcmp(path, "/") == 0)
        path = "/index.html";

    /* fill完整路徑 */
    char full_path[200];
    sprintf(full_path, "page%s", path);

    //printf("%s\n", full_path);

    FILE *fp = fopen(full_path, "rb");

    /*  response req */
    char buff[BUFF_SIZE];
    buff[0] = 0;

    sprintf(buff, "HTTP/1.1 200 OK\r\n");
    write(connfd, buff, strlen(buff));
    sprintf(buff, "\r\n");
    write(connfd, buff, strlen(buff));

    int r = fread(buff, 1, BUFF_SIZE, fp);
    while (r)
    {
        write(connfd, buff, r);
        r = fread(buff, 1, BUFF_SIZE, fp);
    }

    fclose(fp);
}

int main()
{

    signal(SIGINT, sig_int); //ctrl+c

    int connfd;
    char rebuff[BUFF_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    pid_t childpid;

    printf("Creating socket...\n");
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_sys("socket error");

    /*
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *addr;

    getaddrinfo("127.0.0.1", "1450", &hints, &addr);

    servaddr = *(struct sockaddr_in *)(addr->ai_addr);
    */

    /* 114.39.215.45 */

    /*
        設定主機ip_info(較陽春寫法)
        bind() - 綁住socket跟ip
        listen() - 等待cleint端連線
    */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(80);

    printf("Binding socket to local address...\n");
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
        err_sys("bind error");

    printf("Listening...\n");
    listen(listenfd, MAX_CONNECT_NUM);

    signal(SIGCHLD, sig_chld); //擷取子程序結束中斷

    while (1)
    {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

        printf("\nNew connection from %s\n", inet_ntoa(cliaddr.sin_addr));

        if ((childpid = fork()) == 0) //子程序處理就好
        {
            int r = recv(connfd, rebuff, BUFF_SIZE, 0);
            //printf("%d\n", r);

            if (strncmp("GET /", rebuff, 5) == 0)
            {
                printf("Request : GET\n");

                /* 擷取request message */
                char *path = rebuff + 4;
                char *end_path = strstr(path, " ");
                *end_path = '\0';

                printf("Request : %s\n", path);

                response_req(connfd, path);

                printf("Response : %s\n", path);
            }
            else if (strncmp("POST /", rebuff, 6) == 0) //針對post request做處理
            {
                printf("Request : POST\n");
                char buff[BUFF_SIZE];
                strcpy(buff, rebuff);
                char *content = get_file(rebuff);
                strcpy(rebuff, buff);
                //printf("Request : POST\n");
                int length = get_filelen(rebuff);
                strcpy(rebuff, buff);
                //printf("Request : POST\n");
                char *type = get_filetype(rebuff);
                strcpy(rebuff, buff);
                //printf("%ld\n", strlen(content));

                printf("Saving file...\n");

                if (strncmp("image/png", type, 9) == 0) //png
                {
                    printf("Saved file type: image/png\n");
                    printf("Saved file length: %d\n", length);

                    FILE *fp = fopen("page/img1.png", "w+");
                    fseek(fp, 0, SEEK_SET);
                    fwrite(content, sizeof(char), length, fp);
                    fclose(fp);
                }
                else if (strncmp("image/jpeg", type, 10) == 0) //jpg
                {
                    printf("Saved file type: image/jpeg\n");
                    printf("Saved file length: %d\n", length);

                    FILE *fp = fopen("page/jpg1.jpg", "w+");
                    fseek(fp, 0, SEEK_SET);
                    fwrite(content, sizeof(char), length, fp);
                    fclose(fp);
                }
                else if (strncmp("text/plain", type, 10) == 0) //txt
                {
                    printf("Saved file type: text/plain\n");
                    printf("Saved file length: %d\n", length);

                    char *end = strstr(content, "------"); //Bound
                    *end = '\0';

                    FILE *fp = fopen("page/text.txt", "w+");
                    fseek(fp, 0, SEEK_SET);
                    fwrite(content, strlen(content), 1, fp);
                    fclose(fp);
                }

                response_req(connfd, "/"); //更新網頁
            }

            close(connfd);
            exit(0);
        }

        rebuff[0] = '\0';

    } //while(1)

    return 0;
}