#include "nginx.h"

void print(char* s){
    while (*s){
        write(2, s, 1);
        s++;
    }
    write(2, "\n", 1);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void welcome_msg(clt_t* clt, int fd, fd_set* master_set, int nfds){
    char s[37];
    memset(s, 0, sizeof(s));
    sprintf(s, "server: client %d just arrived\n", clt->id);
    for (int i = 0; i < nfds; i++){
        if (FD_ISSET(i, master_set) && i != fd){
            if (send(i, s, strlen(s), 0) == -1){
                print("error");
                continue ;
            }
        }
    }
}

int handle_new_connection(clt_t* clt, int fd, fd_set* master_set, int* nfds){
    struct sockaddr_in add;
    socklen_t s = sizeof(add);
    memset(&add, '\0', sizeof(add));
    int new_cfd = accept(fd, (SOCKADDR*)&add, &s);
    if (new_cfd == -1)
        return (-1);
    clt->fd = new_cfd;
    clt->recv_buff = NULL;
    welcome_msg(clt, fd, master_set, *nfds);
    FD_SET(new_cfd, master_set);
    if (new_cfd + 1 > *nfds)
        *nfds = new_cfd + 1;
    return (clt->fd);
}

void broadcast_message(char* msg, int fd, clt_t* clt, fd_set* master_set, int nfds){
    fprintf(stderr, "clt fd : %d\n", clt->fd);
    for(int i = 0; i < nfds; i++){
        if (FD_ISSET(i, master_set) && i != fd && i != clt->fd){
            if (send(i, msg, strlen(msg), 0) == -1){
                fprintf(stderr, "errno : %d\n", errno);
                fprintf(stderr, "invalid fd : %d\n", i);
                continue ;
            }
        }
    }
}

void clt_disconnect(clt_t* clt, int fd, fd_set* master_set, int nfds){
    FD_CLR(clt->fd, master_set);
    close(clt->fd);
    char s[34];
    memset(s, 0, sizeof(s));
    sprintf(s, "server: client %d just left\n", clt->id);
    for(int i = 0; i < nfds; i++){
        if (FD_ISSET(i, master_set) && i != fd){
            if (send(i, s, strlen(s), 0) == -1){
                continue ;
            }
        }
    }
}

void handle_clients_requests(clt_t* clt, int fd, fd_set* master_set, int nfds){
    char *buff = malloc(sizeof(char) * 1024);
    if (!buff){
        print("Fatal error");
        exit(1);
    }
    char* msg = NULL;
    char prefix[14];
    int bytes_rcvd;
    sprintf(prefix, "client %d: ", clt->id);
    if ((bytes_rcvd = recv(clt->fd, buff, sizeof(buff) - 1, 0)) == -1){
        if (errno == EBADF){
            FD_CLR(clt->fd, master_set);
            close(clt->fd);
        }
        return ;
    }
    if (bytes_rcvd == 0){
        clt_disconnect(clt, fd, master_set, nfds);
        return ;
    } else {
        buff[bytes_rcvd] = '\0';
        clt->recv_buff = str_join(clt->recv_buff, buff);
        while (extract_message(&clt->recv_buff, &msg)){
            broadcast_message(prefix, fd, clt, master_set, nfds);
            broadcast_message(msg, fd, clt, master_set, nfds);
            free(msg);
        }

    }
}

int server_socket(int port){
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr)) == -1){
        print("inet_pton Fatal error");
        exit(1);
    }
    addr.sin_port = htons(port);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1){
        print("socket Fatal error");
        exit(1);
    }
    fcntl(fd, F_SETFD, O_NONBLOCK);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(fd, (SOCKADDR *)&addr, sizeof(addr)) == -1){
        print("bind Fatal error");
        exit(1);
    }
    if (listen(fd, BACKLOG) == -1){
        print("listen Fatal error");
        exit(1);
    }
    return (fd);
}

int main(int ac, char** av){
    if (ac != 2){
        print("Wrong number of arguments");
        exit(1);
    } else {
        int port = atoi(av[1]);
        if (port < 1024){
            print("Invalid port number : Reserved port !");
            exit(1);
        }
        int fd = server_socket(port);
        fprintf(stderr, "server fd : %d\n", fd);
        fd_set master_set;
        fd_set rd_fds;

        int nfds = fd + 1;
        FD_ZERO(&rd_fds);
        FD_ZERO(&master_set);
        FD_SET(fd, &master_set);
        clt_t clts[FD_SETSIZE];
        int fd_to_idx[FD_SETSIZE];
        memset(fd_to_idx, 0, sizeof(fd_to_idx));
        int clt_idx = 0;
        while (1){
            rd_fds = master_set;
            if (select(nfds, &rd_fds, NULL, NULL, 0) == -1){
                if (errno == EINVAL){
                    print("Select Fatal error");
                    exit(1);
                }
                continue ;
            }
            for (int i = 0; i < nfds; i++){
                if (FD_ISSET(i, &rd_fds)){
                    if (i == fd){
                        clts[clt_idx].id = clt_idx;
                        int x = handle_new_connection(clts + clt_idx, fd, &master_set, &nfds);
                        if (x == -1)
                            continue;
                        fd_to_idx[x] = clt_idx;
                        clt_idx++;
                    } else
                        handle_clients_requests(clts + fd_to_idx[i], fd, &master_set, nfds);
                }
            }
        }
    }
    return (0);
} 
