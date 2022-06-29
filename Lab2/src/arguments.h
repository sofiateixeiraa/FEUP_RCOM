#ifndef ARGS_H
#define ARGS_H

#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct arguments{
    char user[128];
    char password[128];
    char host[128];
    char url_path[128];
    char file_name[128];
    char host_name[128];
    char ip[128];
} arguments;

int parseArgs(char *url, arguments *arguments);

int getIp(char *host, arguments *arguments);

int getFileName(arguments *arguments);

#endif