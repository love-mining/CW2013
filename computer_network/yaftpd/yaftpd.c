/*
 * This is part of coursework for Computer Network, 2013.02-2013.06
 * By Pengyu CHEN(cpy.prefers.you@gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 * */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 21
#define LISTEN_QUEUE_SZ 16
#define EOL "\r\n"

typedef struct _yaftp_state_t
{
    
}   yaftp_state_t;

/* SIGCHLD shall be handled or terminated child processes would remain zombie */
static void sig_chld_hndl(int signo)
{
    int status;
    if (wait(&status) < 0)
        perror("error waiting for child process");
    return;
}

int main(int argc, char **argv)
{
    yaftp_state_t yaftp_state;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("failed to create socket");
        exit(1);
    }
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(PORT),
    };
    if (bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("failed to bind socket");
        exit(1);
    }
    if (listen(listen_fd, LISTEN_QUEUE_SZ) < 0)
    {
        perror("failed to listen to socket");
        exit(1);
    }
    
    while (1)
    {
        struct sockaddr saddr;
        int saddr_len = sizeof(saddr);
        int conn_fd = accept(listen_fd, &saddr, &saddr_len);
        if (conn_fd < 0)
        {
            perror("failed to accept connection");
            exit(1);
        }
        pid_t pid = fork();
        if (pid == 0)
        {
            /* child process */
            char *welcome = 
                "220 Hello and, again, welcome to the Aperture Science "
                "computer-aided enrichment center." EOL;
            send(conn_fd, welcome, strlen(welcome), 0);

            const int BUFFSZ = 0x100;
            char buff[BUFFSZ];
            int msglen;
            while (msglen = recv(conn_fd, buff, BUFFSZ, 0) > 0)
                puts(buff);

            close(listen_fd);
            close(conn_fd);
            return 0;
        }
        else if (pid > 0)
        {
            /* parent process */
            close(conn_fd);
        }
        else
        {
            /* failed to fork */
            perror("failed to fork");
            close(conn_fd);
        }
    
    }

    /* execution flow shall not reach here */
    fputs("WARN: This line shall not be executed.", stderr);

    return 0;
}
