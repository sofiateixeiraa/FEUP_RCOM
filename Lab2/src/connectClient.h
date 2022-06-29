#ifndef CONNEC_H
#define CONNEC_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <arpa/inet.h>
#include "arguments.h"


int init(char *ip, int port, int *socketfd);

int clientCommand(int socketfd, char * command);

int passiveMode(int socketfd, char *ip, int *port);

int readResponse(int socketfd, char* rd, size_t size);

int saveFile(int socketfd, char* filename);

#endif