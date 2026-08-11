#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT 	6333
#define MAX_CLIENTS 	30
#define MAX_LEN	        20
#define SERVER_BUFF	100

typedef struct 
{
    char username[MAX_LEN];
    char password[MAX_LEN];
    int status;
    int sockfd;
}user;

typedef struct 
{
    int sockfd;
    struct sockaddr_in client_addr;
}client;

extern user users[MAX_CLIENTS];
extern int user_count;

int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password, int sockfd);

int find_user(const char *username);
void send_online_users(int sockfd);
void broadcast_user_status(const char *username, int status);
void logout_user(int sockfd);

/* Client handling */
void *client_handler(void *arg);

void handle_message(int sender_fd, const char *buffer);
void broadcast_message(int sender_fd, const char *message);
void send_private_message(int sender_fd,const char *receiver,const char *message);

/* Database functions */
int load_users(void);
int save_user(const user *u);
void save_all_users(void);

int get_username_by_fd(int sockfd, char *username);

/* Utility */
void send_response(int sockfd, const char *message);

#endif