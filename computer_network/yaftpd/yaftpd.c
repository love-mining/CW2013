/*
 * This is part of coursework for Computer Network, 2013.02-2013.06
 * By Pengyu CHEN(cpy.prefers.you@gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 * */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libconfig.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EOL "\r\n"

typedef struct _yaftpd_config_t
{
    int port;
    const char *welcome_message;
    int listen_queue_size;
    int inst_buffer_size;
}   yaftpd_config_t;

typedef struct _yaftpd_state_t
{
    yaftpd_config_t *config;
    int listen_fd;
    int inst_conn_fd;
    struct sockaddr saddr;
}   yaftpd_state_t;

/* SIGCHLD shall be handled or terminated child processes would remain zombie */
static void sig_chld_hndl(int signo)
{
    int status;
    if (wait(&status) < 0)
        warn("error waiting for child process");
    return;
}

static void config_file_read(const char *filename, yaftpd_state_t *yaftpd_state)
{
    yaftpd_config_t *config = yaftpd_state->config;
    config_t _cfg, *cfg = &_cfg;
    config_init(cfg);
    if (config_read_file(cfg, filename) == CONFIG_FALSE)
        goto FAIL_EXIT;

    if (config_lookup_int(cfg, "port", &config->port) == CONFIG_FALSE)
        goto FAIL_EXIT;
    if (config_lookup_string(cfg, "welcome_message", &config->welcome_message) == CONFIG_FALSE)
        goto FAIL_EXIT;
    if (config_lookup_int(cfg, "listen_queue_size", &config->listen_queue_size) == CONFIG_FALSE)
        goto FAIL_EXIT;
    if (config_lookup_int(cfg, "inst_buffer_size", &config->inst_buffer_size) == CONFIG_FALSE)
        goto FAIL_EXIT;

    config_destroy(cfg);
    return;

FAIL_EXIT:
    fprintf(stderr, "%s: Failed reading line %d in config file %s: %s\n",
        program_invocation_short_name, 
        config_error_line(cfg),
        config_error_file(cfg),
        config_error_text(cfg));
    exit(1);
    return;
}

static void socket_init(yaftpd_state_t *yaftpd_state)
{
    yaftpd_config_t *yaftpd_config = yaftpd_state->config;
    yaftpd_state->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (yaftpd_state->listen_fd < 0)
        err(1, "Failed to create socket");
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(yaftpd_config->port),
    };
    if (bind(yaftpd_state->listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        err(1, "Failed to bind socket");
    if (listen(yaftpd_state->listen_fd, yaftpd_config->listen_queue_size) < 0)
        err(1, "Failed to listen to socket");
    return;
}

static void yaftpd_session(yaftpd_state_t *yaftpd_state)
{
    yaftpd_config_t *yaftpd_config = yaftpd_state->config;
    int welcome_len = strlen(yaftpd_config->welcome_message);
    send(yaftpd_state->inst_conn_fd, yaftpd_config->welcome_message, welcome_len, 0);

    char buff[yaftpd_config->inst_buffer_size];
    int msglen;
    while (msglen = recv(yaftpd_state->inst_conn_fd, buff, yaftpd_config->inst_buffer_size, 0) > 0)
        puts(buff);
    return;
}

int main(int argc, char **argv)
{
    yaftpd_config_t _config = { };
    yaftpd_state_t _yaftpd_state = { .config = &_config }, *yaftpd_state = &_yaftpd_state;
    config_file_read("yaftpd.conf", yaftpd_state);
    socket_init(yaftpd_state);
    
    /* main loop */
    while (1)
    {
        int saddr_len = sizeof(yaftpd_state->saddr);
        /* this would block until an incoming connection */
        yaftpd_state->inst_conn_fd = accept(yaftpd_state->listen_fd, &yaftpd_state->saddr, &saddr_len);
        if (yaftpd_state->inst_conn_fd < 0)
            err(1, "Failed to accept connection");
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
            /* Failed to fork */
            warn("Failed to fork");
            close(yaftpd_state->inst_conn_fd);
        }
    
    }

    /* execution flow shall not reach here */
    err(1, "This line shall not be executed");

    return 0;
}

