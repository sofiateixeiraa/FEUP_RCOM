#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "arguments.h"


int parseArgs(char *url, arguments *arguments){
    char ftp[] = "ftp://";
    int length = strlen(url);
    int state = 0;
    int i = 0;
    int j = 0;

    while(i < length){
        switch(state){
            case 0:
            if(url[i] == ftp[i] && i < 5) break;
            else if(i == 5 && url[i] == ftp[i]) state = 1;
            else printf("Error: not passing ftp\n");
            break;

            case 1:
            if(url[i] == ':'){
                state = 2;
                j = 0;
            }
            else{
                arguments->user[j] = url[i];
                j++;
            }
            break;

            case 2:
            if(url[i] == '@'){
                state = 3;
                j = 0;
            }
            else{
                arguments->password[j] = url[i];
                j++;
            }
            break;

            case 3:
            if(url[i] == '/'){
                state = 4;
                j = 0;
            }
            else{
                arguments->host[j] = url[i];
                j++;
            }
            break;

            case 4:
            arguments->url_path[j] = url[i];
            j++;
            break;
        }
        i++;
    }
    //case of anonymous user - define user and pass with something random
    if(strcmp(arguments->user, "\0") == 0 || strcmp(arguments->password, "\0") == 0){
        strcpy(arguments->user, "anonymous");
        strcpy(arguments->password, "pass1234");
    }

    if(getIp(arguments->host, arguments) != 0){
        printf("Error getIp()\n");
        return -1;
    }

    if(getFileName(arguments) != 0){
        printf("Error getFileName()\n");
        return -1;
    }
    
    return 0;
    
}

int getIp(char *host, arguments *arguments){
    struct hostent *h;
    
    if((h = gethostbyname(host)) == NULL){
        herror("Error getting host name\n");
        return -1;
    }
    
    strcpy(arguments->host_name, h->h_name);
    strcpy(arguments->ip, inet_ntoa( *( ( struct in_addr *)h->h_addr) )); 

    return 0;
}

int getFileName(arguments *arguments){
    char fullpath[256];

    strcpy(fullpath, arguments->url_path);
    
    char* aux = strtok(fullpath, "/");

    while(aux != NULL){
        strcpy(arguments->file_name, aux);
        aux = strtok(NULL, "/");
    }

    return 0;
}