#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     6333
#define CLIENT_BUFF     100
#define MAX_LEN         20


/* Connection */
int connect_to_server(void);


/* User operations */
void register_user(int sockfd);
int login_user(int sockfd);
void logout_user(int sockfd);


/* Messaging */
void chat_menu(int sockfd);
void send_broadcast_message(int sockfd);
void send_private_message(int sockfd);
void logout_user(int sockfd);

/* Receive messages */
void *receive_messages(void *arg);


/* Menu */
void show_menu(void);

#endif
