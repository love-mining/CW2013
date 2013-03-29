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
    int anonymous_login;
    int anonymous_mask;
    const char *anonymous_root;
}   yaftpd_config_t;

typedef struct _yaftpd_state_t
{
    yaftpd_config_t *config;
    int listen_fd;
    int inst_conn_fd;
    struct sockaddr saddr;
    const char *username;
}   yaftpd_state_t;

/* SIGCHLD shall be handled or terminated child processes would remain zombie */
static void sig_chld_hndl(int signo)
{
    int status;
    if (wait(&status) < 0)
        warn("error waiting for child process");
    return;
}

static void yaftpd_send_str(const char *msg, const yaftpd_state_t *yaftpd_state)
{
    int msglen = strlen(msg);
    send(yaftpd_state->inst_conn_fd, msg, msglen + 1, 0);
    return;
}

static int yaftpd_username_validate(const char *username, yaftpd_state_t *yaftpd_state)
{
    if (yaftpd_state->config->anonymous_login && !strcasecmp(username, "anonymous"))
    {
        return 1;
    }
    else
    {
        /* TODO: general username validate. check if inside ftp group */
    }
    return 0;
}

static void session_response_user(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char username[yaftpd_state->config->inst_buffer_size];
    if (sscanf(instruction, "USER %s", username) < 1)
        yaftpd_send_str("501 Syntax error.\r\n", yaftpd_state);
    else
    {
        if (yaftpd_username_validate(username, yaftpd_state)) 
        {
            yaftpd_state->username = strdup(username);
            yaftpd_send_str("331 User name okay, need password.\r\n", yaftpd_state);
        }
        else
        {
            // TODO: validation failed.
        }
    }
    return;
}

static int yaftpd_password_validate(const char *password, yaftpd_state_t *yaftpd_state)
{
    if (yaftpd_state->config->anonymous_login && !strcasecmp(yaftpd_state->username, "anonymous"))
        return 1;
    else
    {
        /* TODO: general password validate. check with getspnam and getpwnam */
    }
    return 0;
}

static void session_response_pass(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char password[yaftpd_state->config->inst_buffer_size];
    if (sscanf(instruction, "PASS %s", password) < 1)
        yaftpd_send_str("501 Syntax error.\r\n", yaftpd_state);
    else
    {
        if (!yaftpd_state->username)
            yaftpd_send_str("503: Login with USER first.\r\n", yaftpd_state);
        else if (yaftpd_password_validate(password, yaftpd_state))
            yaftpd_send_str("230: User logged in, proceed.\r\n", yaftpd_state);
        else
            yaftpd_send_str("530: Authentication failed.\r\n", yaftpd_state);
    }
    return;
}

typedef struct _session_response_t
{
    const char *instname;
    void (*handler)(const char*, yaftpd_state_t*);
}   session_response_t;

static const session_response_t session_response[] = {
    { "USER", session_response_user },
    { "PASS", session_response_pass },
    { NULL, NULL },
};

static void config_file_read(const char *filename, yaftpd_state_t *yaftpd_state)
{
    yaftpd_config_t *config = yaftpd_state->config;
    config_t _cfg, *cfg = &_cfg;
    config_init(cfg);
    if (config_read_file(cfg, filename) == CONFIG_FALSE)
    {
        fprintf(stderr, "%s: Failed reading line %d in config file %s: %s\n",
            program_invocation_short_name, 
            config_error_line(cfg),
            config_error_file(cfg),
            config_error_text(cfg));
        exit(1);
    }

#define YAFTPD_CONFIG_LOOKUP(type, name)    \
    if (config_lookup_##type(cfg, #name, &config->name) == CONFIG_FALSE)  \
    {   \
        fprintf(stderr, "%s: Failed reading configuration item: %s\n",  \
            program_invocation_short_name, #name); \
        exit(1);    \
    }   \
    if (!strcmp(#type, "string"))   \
        *(const char**)&config->name = strdup(*(const char**)&config->name);
        
    YAFTPD_CONFIG_LOOKUP(int, port);
    YAFTPD_CONFIG_LOOKUP(string, welcome_message);
    YAFTPD_CONFIG_LOOKUP(int, listen_queue_size);
    YAFTPD_CONFIG_LOOKUP(int, inst_buffer_size);
    YAFTPD_CONFIG_LOOKUP(bool, anonymous_login);
    YAFTPD_CONFIG_LOOKUP(int, anonymous_mask);
    YAFTPD_CONFIG_LOOKUP(string, anonymous_root);
#undef YAFTPD_CONFIG_LOOKUP

    config_destroy(cfg);
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
    yaftpd_send_str(yaftpd_config->welcome_message, yaftpd_state);

    char instbuff[yaftpd_config->inst_buffer_size];
    int inst_msg_len;
    while (inst_msg_len = recv(yaftpd_state->inst_conn_fd, instbuff, yaftpd_config->inst_buffer_size, 0))
    {
        /* assuming the maximum instruction size is no more than inst_buffer_size */
        if (inst_msg_len == yaftpd_config->inst_buffer_size)
            fprintf(stderr, "%s: Maximum instruction size exceeded.", program_invocation_short_name);
        instbuff[inst_msg_len] = 0; /* manually added \0 for c strings */

        /* a naive parsing routine. shall use flex+bison instead. */
        int i;
        const session_response_t *sp;
        for (i = 0; sp = &session_response[i], sp->instname; i++)
            if (!strncasecmp(sp->instname, instbuff, strlen(sp->instname)))
            {
                sp->handler(instbuff, yaftpd_state);
                break;
            }
        if (!sp->instname) /* not parsed */
            yaftpd_send_str("502 Command not implemneted.\r\n", yaftpd_state);
            
        printf("inst_msg len: %d\n", inst_msg_len);
        if (inst_msg_len > 0)
            puts(instbuff);
    }
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

