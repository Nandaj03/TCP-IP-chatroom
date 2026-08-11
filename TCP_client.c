#include "TCP_client.h"

int connect_to_server(void)
{
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main()
{
    int sockfd=connect_to_server();
    if(sockfd == -1)
    {
        return -1;
    }

    printf("Connected to server\n");

    int option;

    while(1)
    {
        printf("\n");
        printf("---- CHAT APPLICATION ----\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Enter option: ");

        if(scanf("%d", &option) != 1)
        {
            close(sockfd);
            return 0;
        }

        switch(option)
        {
            case 1:
                register_user(sockfd);
                break;

            case 2:
                if(login_user(sockfd)==1)
                {
                    printf("Login successful\n");
                    pthread_t tid;
                    if(pthread_create(&tid, NULL, receive_messages, &sockfd) != 0)
                    {
                        perror("pthread_create");
                        logout_user(sockfd);
                        close(sockfd);
                        return -1;
                    }
                    chat_menu(sockfd);
                    pthread_join(tid, NULL);
                    close(sockfd);

                    sockfd = connect_to_server();
                    if(sockfd == -1)
                    {
                        return -1;
                    }
                    printf("Connected to server\n");
                }
                break;

            case 3:
                
                close(sockfd);
                return 0;

            default:
                printf("Invalid option\n");
                break;
        }
    }

    close(sockfd);

    return 0;
}

void register_user(int sockfd)
{
    char username[MAX_LEN];
    char password[MAX_LEN];
    char buffer[CLIENT_BUFF];

    printf("Enter username: ");
    if(scanf(" %s", username) != 1)
    {
        return;
    }

    printf("Enter password: ");
    if(scanf(" %s", password) != 1)
    {
        return;
    }

    sprintf(buffer, "1 %s %s",username,password);
    send(sockfd,buffer,strlen(buffer),0);
    memset(buffer, 0, sizeof(buffer));
    int ret = recv(sockfd,buffer,sizeof(buffer) - 1,0);

    if(ret <= 0)
    {
        printf("Server disconnected\n");
        return;
    }
    buffer[ret] = '\0';

    printf("Server: %s", buffer);
}

int login_user(int sockfd)
{
    char username[MAX_LEN];
    char password[MAX_LEN];
    char buffer[CLIENT_BUFF];

    printf("Enter username: ");
    if(scanf("%s", username) != 1)
    {
        return 0;
    }

    printf("Enter password: ");
    if(scanf("%s", password) != 1)
    {
        return 0;
    }

    sprintf(buffer, "2 %s %s",username,password);

    send(sockfd,buffer,strlen(buffer),0);

    memset(buffer, 0, sizeof(buffer));

    int ret = recv(sockfd,buffer,sizeof(buffer) - 1,0);

    if(ret <= 0)
    {
        printf("Server disconnected\n");
        return 0;
    }

    buffer[ret] = '\0';

    printf("Server: %s", buffer);
    if(strstr(buffer, "Logged in successfully") != NULL)
    {
        return 1;
    }
    return 0;
}

void *receive_messages(void *arg)
{
    int sockfd = *(int *)arg;

    char buffer[CLIENT_BUFF];
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));

        int ret = recv(sockfd,buffer,sizeof(buffer) - 1,0);
        if(ret == 0)
        {
            printf("\nServer disconnected\n");
            break;
        }
        if(ret == -1)
        {
            perror("recv");
            break;
        }
        buffer[ret] = '\0';
        printf("\n%s", buffer);
        fflush(stdout);
    }
    return NULL;
}

void chat_menu(int sockfd)
{
    int option;
    while(1)
    {
        printf("\n");
        printf("------ CHAT MENU ------\n");
        printf("1. Group Message\n");
        printf("2. Private Message\n");
        printf("3. Logout\n");
        printf("Enter option: ");
        if(scanf("%d",&option) != 1)
        {
            logout_user(sockfd);
            return;
        }

        switch(option)
        {
            case 1:
                send_broadcast_message(sockfd);
                break;

            case 2:
                send_private_message(sockfd);
                break;

            case 3:
                logout_user(sockfd);
                return;

            default:
                printf("Invalid option\n");
        }   
    }
}

void send_broadcast_message(int sockfd)
{
    char message[CLIENT_BUFF];
    char buffer[CLIENT_BUFF];

    printf("Enter message: ");
    if(scanf(" %97[^\n]", message) != 1)
    {
        return;
    }

    snprintf(buffer, sizeof(buffer), "4 %.97s", message);

    send(sockfd, buffer, strlen(buffer), 0);
}

void send_private_message(int sockfd)
{
    char receiver[MAX_LEN];
    char message[CLIENT_BUFF];
    char buffer[CLIENT_BUFF];

    printf("Enter receiver username: ");
    if(scanf("%19s", receiver) != 1)
    {
        return;
    }

    printf("Enter message: ");
    if(scanf(" %77[^\n]", message) != 1)
    {
        return;
    }

    snprintf(buffer, sizeof(buffer), "5 %.19s %.77s", receiver, message);

    send(sockfd, buffer, strlen(buffer), 0);
}

void logout_user(int sockfd)
{
    char buffer[CLIENT_BUFF];

    snprintf(buffer, sizeof(buffer), "3");

    send(sockfd, buffer, strlen(buffer), 0);
}

