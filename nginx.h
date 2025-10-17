#ifndef NGINX_H
#define NGINX_H

#define _POSIX_C_SOURCE 200112L
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#define BACKLOG 10

typedef struct sockaddr SOCKADDR;
typedef struct clt {
    int id;
    int fd;
    char* recv_buff;    
} clt_t;
#endif