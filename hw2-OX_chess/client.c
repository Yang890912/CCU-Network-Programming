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
#include <pthread.h>

#define PORT 8080
#define MAX_MEMBER 8
#define MAX_BUFF 512
#define MAX_NAME_LEN 32
#define BACKLOG 4

int board[9];
char chess, op_chess;

void print_instr()
{
    printf("Instruction :\n");
    printf("#q #quit : quit game\n");
    printf("#help : show instructions\n");
    printf("#list : list the members and game state\n");
    printf("#start :　create a new game\n");
}

void print_board()
{
    char char_board[9];

    for (int i = 0; i < 9; i++)
    {
        if (board[i] == -1)
            char_board[i] = ' ';
        else if (board[i] == 1)
            char_board[i] = chess;
        else
            char_board[i] = op_chess;
    }

    printf("\n%c : you , %c : opponent\n", chess, op_chess);

    printf(" 0 │ 1 │ 2          %c │ %c │ %c \n", char_board[0], char_board[1], char_board[2]);
    printf("---|---|---        ---|---|---\n");
    printf(" 3 │ 4 │ 5          %c │ %c │ %c \n", char_board[3], char_board[4], char_board[5]);
    printf("---|---|---        ---|---|---\n");
    printf(" 6 │ 7 │ 8          %c │ %c │ %c \n", char_board[6], char_board[7], char_board[8]);
}

void recv_serve(void *n)
{

    int connfd = *(int *)n;
    int in_game = 0;
    char msg_rsv[MAX_BUFF];
    memset(board, -1, sizeof(board)); // Initialize board

    while (1)
    {
        memset(msg_rsv, '\0', MAX_BUFF);

        int len = recv(connfd, msg_rsv, MAX_BUFF, 0);
        msg_rsv[len] = '\0';
        // printf("in_game:%d\n", in_game);
        if (!in_game)
        {
            if (strncmp(msg_rsv, "[SYS", 4) == 0)
            {
                printf("%s", msg_rsv);
                continue;
            }
            else if (strncmp(msg_rsv, "[START]", 7) == 0)
            {
                printf("%s", msg_rsv);
                in_game = 1;
                continue;
            }
        }
        else
        {
            if (strncmp(msg_rsv, "[GAME]", 6) == 0 || strncmp(msg_rsv, "[END]", 5) == 0)
            {
                if (strncmp(msg_rsv, "[END]", 5) == 0)
                    print_board();

                printf("%s", msg_rsv);

                if (strncmp(msg_rsv, "[GAME] You are O", strlen("[GAME] You are O")) == 0)
                    chess = 'O', op_chess = 'X';
                else if (strncmp(msg_rsv, "[GAME] You are X", strlen("[GAME] You are X")) == 0)
                    chess = 'X', op_chess = 'O';
                else if (strncmp(msg_rsv, "[END]", 5) == 0) // End game and initialize state
                    in_game = 0, memset(board, -1, sizeof(board));

                continue;
            }
            else if (strncmp(msg_rsv, "[TURN]", 6) == 0 || strncmp(msg_rsv, "[ERR]", 5) == 0)
            {
                print_board();
                printf("%s", msg_rsv);
                printf("Please enter 0~8\n");
            }
            else
            {
                sscanf(msg_rsv, "%d %d %d %d %d %d %d %d %d", &board[0], &board[1], &board[2], &board[3], &board[4], &board[5], &board[6], &board[7], &board[8]);
            }
        }
    }
}

void cmd(int connfd, const char *head)
{
    char cmd_line[MAX_BUFF];
    //printf("%s", head);

    while (1)
    {
        scanf("%s", cmd_line);
        if (strncmp(cmd_line, "#q", 2) == 0 || strncmp(cmd_line, "#quit", 5) == 0)
        {
            printf("Quit the game\nBye~\n");
            send(connfd, cmd_line, strlen(cmd_line), 0);
            exit(0);
        }
        else if (strncmp(cmd_line, "#help", 5) == 0)
        {
            print_instr();
            continue;
        }

        send(connfd, cmd_line, strlen(cmd_line), 0);
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char username[MAX_NAME_LEN];
    char head[MAX_BUFF];

    if (argc < 2)
    {
        printf("please enter serve IP\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(PORT);

    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        printf("Connecting failed\n"), exit(0);
    else
        printf("Connecting succeed\n");

    printf("Please enter username : ");
    scanf("%s", username);
    printf("[Welocme] Hello %s \n[Welocme] Welcome to this game\r\n", username);
    print_instr();

    // username## <instruction>
    strcpy(head, username);
    strcat(head, "$$ ");

    // Create a thread to receive serve package
    pthread_t thread;
    pthread_create(&thread, NULL, (void *)(&recv_serve), (void *)&connfd);
    sleep(1);

    // upload username to serve
    send(connfd, username, strlen(username), 0);

    // Cmd
    cmd(connfd, head);

    return 0;
}