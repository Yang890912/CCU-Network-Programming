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

int listenfd, client[MAX_MEMBER];
int game_state[MAX_MEMBER];
char member_list[MAX_MEMBER][MAX_NAME_LEN];
int board[MAX_MEMBER][9];

void serve_info()
{
    int i, num = 0;
    printf("This game is made from 408410043\n");
    printf("This port is set to %d\n", PORT);
    printf("The max number of player is %d\n", MAX_MEMBER);

    for (i = 0; i < MAX_MEMBER; i++)
        if (client[i] != -1)
            num++;

    printf("The current number of player is %d\n", num);
    num = 0;

    for (i = 0; i < MAX_MEMBER; i++)
        if (game_state[i] != -1)
            num++;

    printf("The current number of game in progress is %d\n", num / 2);
    printf("-------- end --------\n");
}

void print_instr()
{
    printf("Instruction :\n");
    printf("#q #quit : quit serve\n");
    printf("#show : show info and current state of serve\n");
    printf("#help : show instructions\n");
}

void manage_serve()
{
    char enter[10];
    print_instr();

    while (1)
    {
        scanf("%s", enter); // block to enter instruction to cmd

        if (strncmp(enter, "#q", 2) == 0 || strncmp(enter, "#quit", 5) == 0)
        {
            close(listenfd);
            printf("Serve closed !\n");
            exit(0);
        }
        else if (strncmp(enter, "#show", 5) == 0)
        {
            serve_info();
        }
        else if (strncmp(enter, "#help", 5) == 0)
        {
            print_instr();
        }
        else
        {
            printf("NO this instruction !!! \n");
        }
    }
}

int judge(int p)
{
    if (board[p][0] == 1 && board[p][1] == 1 && board[p][2] == 1)
        return 1;
    if (board[p][3] == 1 && board[p][4] == 1 && board[p][5] == 1)
        return 1;
    if (board[p][6] == 1 && board[p][7] == 1 && board[p][8] == 1)
        return 1;
    if (board[p][0] == 1 && board[p][3] == 1 && board[p][6] == 1)
        return 1;
    if (board[p][1] == 1 && board[p][4] == 1 && board[p][7] == 1)
        return 1;
    if (board[p][2] == 1 && board[p][5] == 1 && board[p][8] == 1)
        return 1;
    if (board[p][0] == 1 && board[p][4] == 1 && board[p][8] == 1)
        return 1;
    if (board[p][2] == 1 && board[p][4] == 1 && board[p][6] == 1)
        return 1;
    return 0;
}

void list_members(char *buff)
{
    memset(buff, '\0', sizeof(buff));
    strcpy(buff, "[SYS] The online member list :\n");

    for (int i = 0; i < MAX_MEMBER; i++)
    {
        if (client[i] != -1)
        {
            strcat(buff, member_list[i]);
            if (game_state[i] == -1)
                strcat(buff, " idle...");
            else
                strcat(buff, " gameing...");
            strcat(buff, "\n");
        }
    }
}

void start_game(int p1, int p2)
{

    int turn = 1;
    int length, allocation;
    char msg_recv[MAX_BUFF];
    char msg_send[MAX_BUFF];
    char msg_wait[] = "[GAME] Waiting for another player's turn\n";
    char msg_turn[] = "[TURN] It's your turn\n";
    char msg_err[] = "[ERR] Illegal allocation !, please enter again\n";
    char msg_win[] = "[END] You are win the game ^^\n";
    char msg_loss[] = "[END] You are loss the game QQ\n";
    char msg_draw[] = "[END] You are draw\n";
    char msg_O[] = "[GAME] You are O\n";
    char msg_X[] = "[GAME] You are X\n";

    char game_info[MAX_BUFF];
    sprintf(game_info, "[ %s vs %s ]", member_list[p1], member_list[p2]);

    for (int i = 0; i < 9; i++)
    {
        board[p1][i] = -1;
        board[p2][i] = -1;
    }

    send(client[p1], msg_O, strlen(msg_O), 0);
    send(client[p2], msg_X, strlen(msg_X), 0);
    usleep(10000);
    while (turn)
    {
        memset(msg_recv, '\0', sizeof(msg_recv));
        memset(msg_send, '\0', sizeof(msg_send));

        // Draw
        if (turn == 10)
        {
            printf("%s The game is draw\n", game_info);
            send(client[p1], msg_draw, strlen(msg_draw), 0);
            send(client[p2], msg_draw, strlen(msg_draw), 0);
            break;
        }

        if (turn % 2 == 1) // p1 turn
        {
            send(client[p1], msg_turn, strlen(msg_turn), 0); // p1 enter
            send(client[p2], msg_wait, strlen(msg_wait), 0); // p2 wait
            usleep(10000);
            while (1)
            {
                usleep(10000);
                length = recv(client[p1], msg_recv, MAX_BUFF, 0);
                msg_recv[length] = '\0';
                allocation = atoi(msg_recv);
                // Illegal allocation
                if (allocation > 8 || allocation < 0)
                {
                    send(client[p1], msg_err, strlen(msg_err), 0);
                    continue;
                }
                else if (board[p1][allocation] != -1 || board[p2][allocation] != -1)
                {
                    send(client[p1], msg_err, strlen(msg_err), 0);
                    continue;
                }
                break;
            }
            // 1 denote self allocation, 0 denote another allocation
            board[p1][allocation] = 1;
            board[p2][allocation] = 0;
            usleep(10000);
            // renew respective board table
            sprintf(msg_send, "%d %d %d %d %d %d %d %d %d", board[p1][0], board[p1][1], board[p1][2], board[p1][3], board[p1][4], board[p1][5], board[p1][6], board[p1][7], board[p1][8]);
            send(client[p1], msg_send, strlen(msg_send), 0);
            sprintf(msg_send, "%d %d %d %d %d %d %d %d %d", board[p2][0], board[p2][1], board[p2][2], board[p2][3], board[p2][4], board[p2][5], board[p2][6], board[p2][7], board[p2][8]);
            send(client[p2], msg_send, strlen(msg_send), 0);
            usleep(10000);
            // p1 win
            if (judge(p1))
            {
                printf("%s %s win the game !\n", game_info, member_list[p1]);
                printf("%s %s loss the game !\n", game_info, member_list[p2]);
                send(client[p1], msg_win, strlen(msg_win), 0);
                send(client[p2], msg_loss, strlen(msg_loss), 0);
                break;
            }
        }
        else // p2 turn
        {
            send(client[p2], msg_turn, strlen(msg_turn), 0); // p2 enter
            send(client[p1], msg_wait, strlen(msg_wait), 0); // p1 wait

            while (1)
            {
                usleep(10000);
                length = recv(client[p2], msg_recv, MAX_BUFF, 0);
                msg_recv[length] = '\0';
                allocation = atoi(msg_recv);
                // Illegal allocation
                if (allocation > 8 || allocation < 0)
                {
                    send(client[p2], msg_err, strlen(msg_err), 0);
                    continue;
                }
                else if (board[p2][allocation] != -1 || board[p1][allocation] != -1)
                {
                    send(client[p2], msg_err, strlen(msg_err), 0);
                    continue;
                }
                break;
            }
            // 1 denote self allocation, 0 denote another player's allocation
            board[p2][allocation] = 1;
            board[p1][allocation] = 0;
            usleep(10000);
            // renew respective board table
            sprintf(msg_send, "%d %d %d %d %d %d %d %d %d", board[p2][0], board[p2][1], board[p2][2], board[p2][3], board[p2][4], board[p2][5], board[p2][6], board[p2][7], board[p2][8]);
            send(client[p2], msg_send, strlen(msg_send), 0);
            sprintf(msg_send, "%d %d %d %d %d %d %d %d %d", board[p1][0], board[p1][1], board[p1][2], board[p1][3], board[p1][4], board[p1][5], board[p1][6], board[p1][7], board[p1][8]);
            send(client[p1], msg_send, strlen(msg_send), 0);
            usleep(10000);
            // p2 win
            if (judge(p2))
            {
                printf("%s %s win the game !\n", game_info, member_list[p2]);
                printf("%s %s loss the game !\n", game_info, member_list[p1]);
                send(client[p2], msg_win, strlen(msg_win), 0);
                send(client[p1], msg_loss, strlen(msg_loss), 0);
                break;
            }
        }

        turn++;

    } // while(turn)

    game_state[p1] = -1;
    game_state[p2] = -1;
    printf("%s The game end!\n", game_info);

    return;
}

void manage_client(void *n)
{
    int id = *(int *)n;
    if (id >= MAX_MEMBER)
        pthread_exit(NULL);

    char msg_recv[MAX_BUFF];
    char msg_send[MAX_BUFF];
    char msg[MAX_BUFF];
    char member_name[MAX_NAME_LEN];
    char opponent[MAX_NAME_LEN];

    char msg_invite[] = "[SYS] Who you want to invite to this game ?\n";
    char msg_ask[] = "[SYS] Enter \"y\" or \"Y\" if you want to play with";
    char msg_reject[] = "[SYS] Player reject your invivaton\n";
    char msg_accept[] = "[SYS] Player accept your invivaton\n";
    char msg_noexist[] = "[SYS_ERR] No exist player you invite\n";
    char msg_ingame[] = "[SYS_ERR] Player you invite is already in game\n";
    char msg_startgame[] = "[START] New game start\n";

    int opidx;

    // Receive client name
    int name_len = recv(client[id], member_name, MAX_NAME_LEN, 0);
    if (name_len > 0) // Join name to member_list
    {
        member_name[name_len] = '\0';
        strcpy(member_list[id], member_name);

        printf("Player %s join the game\n", member_name);
    }

    while (1)
    {
        // Initialize buffers
        memset(msg_recv, '\0', sizeof(msg_recv));
        memset(msg_send, '\0', sizeof(msg_send));
        memset(msg, '\0', sizeof(msg));
        memset(opponent, '\0', sizeof(opponent));

        // If client n is Gameing ... skip recv
        if (game_state[id] != -1)
        {
            usleep(100);
            continue;
        }

        // Receive client req
        int message_len = recv(client[id], msg_recv, MAX_BUFF, 0);
        msg_recv[message_len] = '\0';
        if (message_len > 0)
        {

            if (strcmp(msg_recv, "#q") == 0 || strcmp(msg_recv, "#quit") == 0)
            { // Quit req
                printf("Player %s exit the game\n", member_name);
                close(client[id]);
                client[id] = -1;
                pthread_exit(NULL);
            }
            else if (strcmp(msg_recv, "#list") == 0)
            { // List members in game
                list_members(msg_send);
                send(client[id], msg_send, strlen(msg_send), 0);
            }
            else if (strncmp(msg_recv, "#start", 6) == 0)
            { //Create a new game

                // Look up client n already in the game, fundationally, this shoud not execute
                if (game_state[id] != -1)
                {
                    strcpy(msg_send, "You are in a game, it's illegal operation\n");
                    send(client[id], msg_send, strlen(msg_send), 0); // Send illegal
                    continue;
                }

                printf("\n%s invites ", member_name);

                // Which client want to invite
                send(client[id], msg_invite, strlen(msg_invite), 0);
                int name_len = recv(client[id], opponent, MAX_NAME_LEN, 0);
                opponent[name_len] = '\0';
                printf("%s to play OX-chess game\n", opponent);

                // Ask opponent for join
                sprintf(msg_send, "%s %s\n", msg_ask, member_name);

                // Find opponent's socket and invite to
                int i;
                for (i = 0; i < MAX_MEMBER; i++)
                {
                    if (client[i] != -1 && strcmp(opponent, member_list[i]) == 0)
                    {
                        if (game_state[i] == -1)
                        {
                            printf("Request...\n");
                            send(client[i], msg_send, strlen(msg_send), 0); // Send invitation
                            opidx = i;
                            game_state[id] = opidx;
                            game_state[opidx] = id;
                            break;
                        }
                        else
                        {
                            // Failed
                            printf("%s is already in the game\n", opponent);
                            send(client[id], msg_ingame, strlen(msg_ingame), 0); // Send in game
                            break;
                        }
                    }
                }
                // Failed
                if (i == MAX_MEMBER)
                    printf("No exist player name\n"), send(client[id], msg_noexist, strlen(msg_noexist), 0); // Send no player

                // Failed
                if (game_state[id] == -1)
                {
                    printf("Game failed\n");
                    continue;
                }

                // Response from player
                int length = recv(client[opidx], msg, MAX_BUFF, 0);
                msg[length] = '\0';
                printf("response\n");

                // Reject
                if (strncmp(msg, "y", 1) != 0 && strncmp(msg, "Y", 1) != 0)
                {
                    printf("%s reject the invitation\n", opponent);
                    game_state[opidx] = -1;
                    game_state[id] = -1;
                    send(client[id], msg_reject, strlen(msg_reject), 0); // Send reject
                    continue;
                }
                // Accept
                send(client[id], msg_accept, strlen(msg_accept), 0);
                printf("%s accept the invitation\n", opponent);
                printf("Game start : %s vs %s\n", member_name, opponent);
                // printf("Waiting...\n");
                sleep(2); // 不能太快 下面的send會影響到 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // Remind
                send(client[id], msg_startgame, strlen(msg_startgame), 0);
                send(client[opidx], msg_startgame, strlen(msg_startgame), 0);
                sleep(2);
                start_game(id, opidx);
            }
            // else
            // {
            //     send(client[game_state[id]], msg_recv, strlen(msg_recv), 0); // 子線程傳給子線程
            // }
        }
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    printf("Creating socket...\n");
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket error\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    printf("Binding socket to local address...\n");
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        printf("Bind error\n");
        exit(0);
    }

    printf("Listening...\r\n");
    listen(listenfd, BACKLOG);

    // Create a thread to manage serve
    pthread_t thread;
    pthread_create(&thread, NULL, (void *)&manage_serve, NULL);

    // Initialize game_state & client table & member_list
    memset(game_state, -1, sizeof(game_state));
    memset(client, -1, sizeof(client));
    memset(member_list, '\0', sizeof(member_list));

    while (1)
    {
        int i;
        clilen = sizeof(cliaddr);
        for (i = 0; (client[i] != -1) && (i < MAX_MEMBER); i++)
            ;

        if (i < MAX_MEMBER)
            client[i] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

        // if (client[i] == -1)
        //     exit(0);

        pthread_create(malloc(sizeof(pthread_t)), NULL, (void *)(&manage_client), (void *)&i);
        usleep(100);
    }

    return 0;
}