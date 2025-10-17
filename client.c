#include "nginx.h"

int main(){
    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    if (inet_pton(addr.sin_family, "127.0.0.1", &addr.sin_addr) == -1){
        perror(strerror(errno));
        exit(1);
    }
    int s_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_fd == -1){
        perror("error");
        exit(1);
    }
    if (connect(s_fd, (SOCKADDR*)&addr, sizeof(addr)) == -1){
        perror("error");
        exit(1);
    }
    // if (send(s_fd, "heeeey hello", strlen("heeeey hello"), 0) == -1){
    //     perror("error");
    //     exit(1);
    // }
    // fprintf(stdout, "Client connected succesfully !\n");
    // close(s_fd);
}