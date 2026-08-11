#include "TCP_server.h"

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
user users[MAX_CLIENTS];
int user_count = 0;

static void disconnect_user(int sockfd, int notify_client);

int main()
{
    if(load_users() == -1)
    {
        printf("Failed to load users\n");
        return -1;
    }
    printf("%d users loaded\n",user_count);
    //socket
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr,client_info;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(SERVER_PORT);
    server_addr.sin_addr.s_addr=inet_addr(SERVER_IP);

    //bind
    int ret=bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));

    if(ret == -1)
	{
		perror("bind");
		return -1;
	}

    //listen
    ret=listen(sockfd,MAX_CLIENTS);
    if(ret==-1)
    {
        perror("listen");
        return -1;
    }
    socklen_t client_len=sizeof(client_info);
    while(1)
    {
        int *client_fd=malloc(sizeof(int));
        if(client_fd == NULL)
        {
            perror("malloc");
            continue;
        }

        client_len=sizeof(client_info);
        *client_fd=accept(sockfd,(struct sockaddr*)&client_info,&client_len);
        if(*client_fd == -1)
        {
            perror("accept");
            free(client_fd);
            continue;
        }
        
        //create thread
        pthread_t tid;
        if(pthread_create(&tid,NULL,client_handler,client_fd)!=0)
        {
            perror("pthread_create\n");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(tid);

    }
}

void *client_handler(void *arg)
{
    int client_fd=*(int*)arg;
    free(arg);
    printf("client handler started\n");
    while(1)
    {
        char buffer[SERVER_BUFF];
        memset(buffer,0,sizeof(buffer));
       int ret= recv(client_fd,buffer,sizeof(buffer)-1,0);
       if(ret==0)
       {
        printf("client disconnected\n");
        disconnect_user(client_fd,0);
        break;
       }
       if(ret==-1)
       {
        perror("recv");
        disconnect_user(client_fd,0);
        break;
       }
        buffer[ret]='\0';
       //client sends data like option other datas
       int option;
       char username[100],password[100];
        if(sscanf(buffer, "%d", &option) != 1)
        {
            printf("Invalid request\n");
            continue;
        }

        if(option == 1 || option == 2)
        {
            if(sscanf(buffer, "%d %19s %19s",
                    &option, username, password) != 3)
            {
                printf("Invalid request\n");
                continue;
            }
        }

    //    sscanf(buffer,"%d %s %s",&option,username,password);
       switch(option)
       {
        case 1:
        {
                int ret=register_user(username,password);
                
                if(ret == 1)
                {
                    printf("User registration successful\n");
                    send_response(client_fd, "User registration successful\n");
                }
                else if(ret == 0)
                {
                    printf("User already registered\n");
                    send_response(client_fd, "User already registered\n");

                }
                else
                {
                    printf("User database full\n");
                    send_response(client_fd,"User database full\n" );
                }
                break;
        }
        case 2:
        {
                int ret=login_user(username,password,client_fd);
                if(ret==1)
                    printf("Logged in successfully");
                    
                else
                    printf("Logged in unsuccessfull");
                break;
        }
        case 3:
            {
                logout_user(client_fd);
                close(client_fd);
                return NULL;
            }
        case 4:
        {
            handle_message(client_fd, buffer);
            break;
        }   
        case 5:
        {
            handle_message(client_fd,buffer);
            break;

        }

       }


    }
    close(client_fd);
    return NULL;
}

int find_user(const char *username)
{
    for(int i=0;i<user_count;i++)
    {
        if(strcmp(users[i].username,username)==0)
        {
            return i;
        }
    }
    return -1;
}

int register_user(const char *username, const char *password)
{
    pthread_mutex_lock(&user_mutex);
    int ret=find_user(username);
    if(ret!=-1)
    {
        pthread_mutex_unlock(&user_mutex);
        return 0;
    }

    if(user_count >= MAX_CLIENTS)
    {
        pthread_mutex_unlock(&user_mutex);
        return -1;      // database full
    }
    strcpy(users[user_count].username,username);
    strcpy(users[user_count].password,password);
    users[user_count].status=0;
    users[user_count].sockfd=-1;

    user_count++;
    if(save_user(&users[user_count - 1]) == -1)
    {
        user_count--;
        pthread_mutex_unlock(&user_mutex);
        return -1;
    }
    pthread_mutex_unlock(&user_mutex);
    return 1;
}

int login_user(const char *username, const char *password, int sockfd)
{
    pthread_mutex_lock(&user_mutex);
    int index=find_user(username);
    if(index==-1)
    {
        printf("user not registered\n");

        send_response(sockfd, "User-name not found\n");
        pthread_mutex_unlock(&user_mutex);
        return -1;
    }
    if(strcmp(users[index].password,password)!=0)
    {
        printf("password incorrect\n");
        send_response(sockfd, "Password not matching\n");
        pthread_mutex_unlock(&user_mutex);
        return -1;
    }
    users[index].status=1;
    users[index].sockfd=sockfd;
    send_response(sockfd, "Logged in successfully\n");
    pthread_mutex_unlock(&user_mutex);
    send_online_users(users[index].sockfd);

    broadcast_user_status(username,1);
    save_all_users();
    return 1;


}

void send_online_users(int sockfd)
{
    pthread_mutex_lock(&user_mutex);
    char buff[SERVER_BUFF];
    memset(buff,0,sizeof(buff));
    strcat(buff,"Online users:\n");

    for(int i=0;i<user_count;i++)
    {
        if(users[i].status==1)  
        {
            strcat(buff,users[i].username);
            strcat(buff,"\n");
        }
    }
    send(sockfd,buff,strlen(buff),0);
    pthread_mutex_unlock(&user_mutex);


}

void broadcast_user_status(const char *username, int status)
{
    pthread_mutex_lock(&user_mutex);
    char buff[SERVER_BUFF];
    memset(buff,0,sizeof(buff));

    if(status==1)
    {
        sprintf(buff,"%s joined the chat",username);

    }
    else
    {
        sprintf(buff,"%s left the chat",username);
    }

    for(int i=0;i<user_count;i++)
    {
        if(users[i].status==1 && strcmp(users[i].username,username)!=0)
        {
            send(users[i].sockfd,buff,strlen(buff),0);
        }
    }
    pthread_mutex_unlock(&user_mutex);
}

void send_response(int sockfd, const char *message)
{
    send(sockfd, message, strlen(message), 0);
}

void logout_user(int sockfd)
{
    disconnect_user(sockfd,1);
}

static void disconnect_user(int sockfd, int notify_client)
{
    pthread_mutex_lock(&user_mutex);
    for(int i=0;i<user_count;i++)
    {
        if(users[i].sockfd==sockfd)
        {
            char username[MAX_LEN];
            strcpy(username,users[i].username);

            if(notify_client)
            {
                send_response(users[i].sockfd,"logged out successfully\n");
            }
            users[i].status=0;
            users[i].sockfd=-1;
            pthread_mutex_unlock(&user_mutex);
            broadcast_user_status(username,0);
            save_all_users();
            return;
        }
    }
    if(notify_client)
    {
        send_response(sockfd,"logged out unsuccessful\n");
    }
    pthread_mutex_unlock(&user_mutex);
}

int save_user(const user *u)
{
    int fd=open("database.txt",O_WRONLY | O_CREAT | O_APPEND,0644);
    if(fd==-1)
    {
        perror("open");
        return -1;
    }
    char buff[100];
    sprintf(buff,"%s %s %d %d\n",u->username,u->password,u->status,u->sockfd);
    write(fd,buff,strlen(buff));
    close(fd);
    return 0;

}

int load_users(void)
{
    FILE *fp = fopen("database.txt", "r");

    if(fp == NULL)
    {
        if(errno == ENOENT)
            return 0;

        perror("fopen");
        return -1;
    }
    user_count = 0;

    while(user_count < MAX_CLIENTS &&
          fscanf(fp, "%19s %19s %d %d\n",
                 users[user_count].username,
                 users[user_count].password,
                 &users[user_count].status,
                 &users[user_count].sockfd) == 4)
    {
        users[user_count].status = 0;
        users[user_count].sockfd = -1;

        user_count++;
    }

    fclose(fp);

    return 0;
}

void save_all_users(void)
{
    FILE *fp = fopen("database.txt", "w");

    if(fp == NULL)
    {
        perror("fopen");
        return;
    }

    for(int i = 0; i < user_count; i++)
    {
        fprintf(fp, "%s %s %d %d\n",
                users[i].username,
                users[i].password,
                users[i].status,
                users[i].sockfd);
    }

    fclose(fp);
}

void broadcast_message(int sender_fd, const char *message)
{
    char username[MAX_LEN];
    char buff[SERVER_BUFF];

    if(get_username_by_fd(sender_fd,username)==0)
    {
        send_response(sender_fd, "You are not logged in\n");
        return;
    }
    sprintf(buff,"%s: %s\n", username, message);

    pthread_mutex_lock(&user_mutex);
    for(int i=0;i<user_count;i++)
    {
        if(users[i].status==1  && users[i].sockfd!=sender_fd)
        {
            send(users[i].sockfd,buff,strlen(buff),0);
        }
    }
    pthread_mutex_unlock(&user_mutex);
}

void handle_message(int sender_fd, const char *buffer)
{
    char message[SERVER_BUFF];
        char receiver[MAX_LEN];
    int option;

    if(sscanf(buffer, "%d", &option) != 1)
    {
        send_response(sender_fd, "Invalid message\n");
        return;
    }

    if(option==4)
    {
        if(sscanf(buffer,"%d %[^\n]",&option,message)!=2)
            {
                send_response(sender_fd,"invalid message\n");
                return;
            }
        broadcast_message(sender_fd,message);
    }
    else if(option==5)
    {
        if(sscanf(buffer, "%d %s %[^\n]",&option, receiver, message) != 3)
        {
            send_response(sender_fd, "Invalid private message\n");
            return;
        }
        send_private_message(sender_fd,receiver,message);
    }
}

int get_username_by_fd(int sockfd, char *username)
{
    pthread_mutex_lock(&user_mutex);

    for(int i=0;i<user_count;i++)
    {
        if(users[i].sockfd==sockfd)
        {
            strcpy(username,users[i].username);
            pthread_mutex_unlock(&user_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&user_mutex);
    return 0;

}

void send_private_message(int sender_fd,const char *receiver,const char *message)
{
    pthread_mutex_lock(&user_mutex);
    int index = find_user(receiver);

    if(index == -1)
    {
        pthread_mutex_unlock(&user_mutex);

        send_response(sender_fd,"User not found\n");
        return;
    }
    if(users[index].status != 1)
    {
        pthread_mutex_unlock(&user_mutex);

        send_response(sender_fd,"User is offline\n");
        return;
    }

    char sender_name[MAX_LEN];

    int found = 0;

    for(int i = 0; i < user_count; i++)
    {
        if(users[i].sockfd == sender_fd)
        {
            strcpy(sender_name, users[i].username);
            found = 1;
            break;
        }
    }

    if(found==0)
    {
        pthread_mutex_unlock(&user_mutex);

        send_response(sender_fd,"You are not logged in\n");
        return;
    }
     char buff[SERVER_BUFF];

    sprintf(buff,"[Private] %s: %s\n",sender_name, message);
    send(users[index].sockfd, buff,strlen(buff),0);
        pthread_mutex_unlock(&user_mutex);
}
