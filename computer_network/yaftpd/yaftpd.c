/*
 * This is part of coursework for Computer Network, 2013.02-2013.06
 * By Pengyu CHEN(cpy.prefers.you@gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 * */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 21
#define LISTEN_QUEUE_SZ 16
#define EOL "\r\n"

typedef struct _yaftpd_state_t
{
    int listen_fd;
    int inst_conn_fd;
    struct sockaddr saddr;
}   yaftpd_state_t;

/* SIGCHLD shall be handled or terminated child processes would remain zombie */
static void sig_chld_hndl(int signo)
{
    int status;
    if (wait(&status) < 0)
        perror("error waiting for child process");
    return;
}

static void config_file_read(const char *filename, yaftpd_state_t *yaftpd_state)
{
    config_t _config, *config = &_config;
    config_init(config);

    config_read_file(config, filename);
    int port;
    config_lookup_int(config, "port", &port);
    printf("port: %d\n", port);

    config_destroy(config);
    return;
}

static void socket_init(yaftpd_state_t *yaftpd_state)
{
    yaftpd_state->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (yaftpd_state->listen_fd < 0)
    {
        perror("failed to create socket");
        exit(1);
    }
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(PORT),
    };
    if (bind(yaftpd_state->listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("failed to bind socket");
        exit(1);
    }
    if (listen(yaftpd_state->listen_fd, LISTEN_QUEUE_SZ) < 0)
    {
        perror("failed to listen to socket");
        exit(1);
    }
    return;
}

static void yaftpd_session(yaftpd_state_t *yaftpd_state)
{
    const char *welcome = 
        "220 Hello and, again, welcome to the Aperture Science "
        "computer-aided enrichment center." EOL;
    send(yaftpd_state->inst_conn_fd, welcome, strlen(welcome), 0);

    const int BUFFSZ = 0x100;
    char buff[BUFFSZ];
    int msglen;
    while (msglen = recv(yaftpd_state->inst_conn_fd, buff, BUFFSZ, 0) > 0)
        puts(buff);
    return;
}

int main(int argc, char **argv)
{
    yaftpd_state_t _yaftpd_state = {}, *yaftpd_state = &_yaftpd_state;
    config_file_read("yaftpd.conf", NULL);
    socket_init(yaftpd_state);
    
    /* main loop */
    while (1)
    {
        int saddr_len = sizeof(yaftpd_state->saddr);
        /* this would block until an incoming connection */
        yaftpd_state->inst_conn_fd = accept(yaftpd_state->listen_fd, &yaftpd_state->saddr, &saddr_len);
        if (yaftpd_state->inst_conn_fd < 0)
        {
            perror("failed to accept connection");
            exit(1);
        }
        pid_t pid = fork();
        if (pid == 0)
        {
            /* child process */
            yaftpd_session(yaftpd_state);
            close(yaftpd_state->listen_fd);
            close(yaftpd_state->inst_conn_fd);
            return 0;
        }
        else if (pid > 0)
        {
            /* parent process */
            /* nothing to do */
            close(yaftpd_state->inst_conn_fd);
        }
        else
        {
            /* failed to fork */
            perror("failed to fork");
            close(yaftpd_state->inst_conn_fd);
        }
    
    }

    /* execution flow shall not reach here */
    fputs("WARN: This line shall not be executed.", stderr);

    return 0;
}

