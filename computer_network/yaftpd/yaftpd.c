/*
 * This is part of coursework for Computer Network, 2013.02-2013.06
 * By Pengyu CHEN(cpy.prefers.you@gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 * */

#define _POSIX_C_SOURCE 200809L 
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libconfig.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum _YAFTPD_DATA_CONN_MODE
{
    YAFTPD_ACTIVE = 1,
    YAFTPD_PASSIVE,
};

typedef struct _yaftpd_config_t
{
    int inst_port;
    int data_port;
    const char *welcome_message;
    int listen_queue_size;
    int inst_buffer_size;
    int anonymous_login;
    int anonymous_mask;
    const char *anonymous_root;
    int listing_line_size;
}   yaftpd_config_t;

typedef struct _yaftpd_state_t
{
    yaftpd_config_t *config;
    int inst_listen_fd;
    int inst_conn_fd;
    int data_conn_mode;
    int data_listen_fd;
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

static int socket_listen(yaftpd_state_t *yaftpd_state, int inaddr, int port, int *fd)
{
    yaftpd_config_t *yaftpd_config = yaftpd_state->config;
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd < 0)
    {
        warn("error creating socket");
        return -1;
    }
    int tmpint = 1;
    if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &tmpint, sizeof(tmpint)) < 0)
    {
        warn("error setting socket reuse");
        return -1;
    }
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(inaddr),
        .sin_port = htons(port),
    };
    if (bind(*fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        warn("error binding socket");
        return -1;
    }
    if (listen(*fd, yaftpd_config->listen_queue_size) < 0)
    {
        warn("error listening to socket");
        return -1;
    }
    return 0;
}

static void socket_send_fmtstr(int conn_fd, const char *fmt, ...)
{
    va_list args;
    char *msg;
    va_start(args, fmt);
    vasprintf(&msg, fmt, args);
    int msglen = strlen(msg);
    send(conn_fd, msg, msglen, 0);
    free(msg);
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
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
    else
    {
        if (yaftpd_username_validate(username, yaftpd_state)) 
        {
            yaftpd_state->username = strdup(username);
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "331 User name okay, need password.\r\n");
        }
        else
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "530 Invalid user name.\r\n");
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
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
    else
    {
        if (!yaftpd_state->username)
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "503 Login with USER first.\r\n");
        else if (yaftpd_password_validate(password, yaftpd_state))
        {
            if (!strcasecmp(yaftpd_state->username, "anonymous"))
            {
                chdir(yaftpd_state->config->anonymous_root);
                //chroot(yaftpd_state->config->anonymous_root);
                //umask(yaftpd_state->config->anonymous_mask);
            }
            else
            {
                /* TODO: chroot for normal users */
            }
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "230 User logged in, proceed.\r\n");
        }
        else
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "530 Authentication failed.\r\n");
    }
    return;
}

static void session_response_pwd(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char *pwd = get_current_dir_name();
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "257 \"%s\"\r\n", pwd);
    free(pwd);
    return;
}

static void session_response_type(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char typecode;
    if (sscanf(instruction, "TYPE %c", &typecode) < 1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
    switch(typecode)
    {
    case 'I':   /* Image(binary files) */
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "200 Command okay.\r\n");
        break;
    deafault:   /* not impelmented */
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "504 Command not impelmented for that parameter.\r\n");
        break;
    }
    return;
}

static void session_response_pasv(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    if (yaftpd_state->data_listen_fd > 0)
    {
        close(yaftpd_state->data_listen_fd);
        yaftpd_state->data_listen_fd = -1;
    }

    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    if (getsockname(yaftpd_state->inst_conn_fd, &addr, &addrlen))
    {
        warn("error getting socket name");
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open passive connection.\r\n");
        return;
    }
    int iaddr[4];
    sscanf(inet_ntoa(addr.sin_addr), "%d.%d.%d.%d", 
        &iaddr[0], &iaddr[1], &iaddr[2], &iaddr[3]);
    yaftpd_state->config->data_port = rand() % 32768 + 1024;
    int port = yaftpd_state->config->data_port; 
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n",
        iaddr[0], iaddr[1], iaddr[2], iaddr[3], port >> 8, port & 255);
    yaftpd_state->data_conn_mode = YAFTPD_PASSIVE;

    if (socket_listen(yaftpd_state, INADDR_ANY, yaftpd_state->config->data_port, &yaftpd_state->data_listen_fd))
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open passive connection.\r\n");
        return;
    }
    
    return;
}

static void session_response_list(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "LIST %s", param) < 1)
        target = "";

    int data_conn_fd;
    /* preparing the data connection */
    if (yaftpd_state->data_conn_mode == YAFTPD_ACTIVE)
    {
        /* TODO: implement this */
        /* this shall not happen for now */
        err(1, "active mode not implmented yet");
    }
    else if (yaftpd_state->data_conn_mode == YAFTPD_PASSIVE)
    {
        if (yaftpd_state->data_listen_fd < 0)
        {
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
            return;
        }
        struct sockaddr dsaddr;        
        int saddr_len = sizeof(dsaddr);
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "150 Opening data connection.\r\n");
        
        /* this would block until an incoming connection */
        data_conn_fd = accept(yaftpd_state->data_listen_fd, &dsaddr, &saddr_len);
        if (data_conn_fd < 0)
        {
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
            return;
        }
    }
    else
    {
        /* data transfer mode not specified. */
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
    }
        
    /* do the listing and send data */
    char *command;
    asprintf(&command, "ls -la %s", target);
    FILE *pfin = popen(command, "r");

    int line_size = yaftpd_state->config->listing_line_size;
    char listing_line[line_size];
    /* read the "total xx" line */
    fgets(listing_line, line_size, pfin);
    while (fgets(listing_line, line_size, pfin))
    {
        listing_line[strlen(listing_line) - 1] = 0; /* overwrite the newline */
        socket_send_fmtstr(data_conn_fd, "%s\r\n", listing_line);
    }

    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "226 Closing data connection.\r\n");
    close(data_conn_fd);
    pclose(pfin);
    free(command);
    return;
}

static void session_response_cwd(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "CWD %s", param) < 1)
        target = "/";

    if (chdir(target))
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 No such directory.\r\n");
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "250 Command okay.\r\n");

    return;
}

typedef struct _session_response_t
{
    const char *instname;
    void (*handler)(const char*, yaftpd_state_t*);
}   session_response_t;

static const session_response_t session_response[] = {
    { "USER",   session_response_user },
    { "PASS",   session_response_pass },
    { "PWD",    session_response_pwd },
    { "TYPE",   session_response_type },
    { "PASV",   session_response_pasv },
    // { "PORT",   NULL }, /* TODO: implement this */
    { "LIST",   session_response_list },
    { "CWD",    session_response_cwd },
    { NULL,     NULL },
};

static void config_file_read(const char *filename, yaftpd_state_t *yaftpd_state)
{
    yaftpd_config_t *config = yaftpd_state->config;
    config_t _cfg, *cfg = &_cfg;
    config_init(cfg);
    if (config_read_file(cfg, filename) == CONFIG_FALSE)
    {
        fprintf(stderr, "%s: error reading line %d in config file %s: %s\n",
            program_invocation_short_name, 
            config_error_line(cfg),
            config_error_file(cfg),
            config_error_text(cfg));
        exit(1);
    }

#define YAFTPD_CONFIG_LOOKUP(type, name)    \
    if (config_lookup_##type(cfg, #name, &config->name) == CONFIG_FALSE)  \
    {   \
        fprintf(stderr, "%s: error reading configuration item: %s\n",  \
            program_invocation_short_name, #name); \
        exit(1);    \
    }   \
    if (!strcmp(#type, "string"))   \
        *(const char**)&config->name = strdup(*(const char**)&config->name);
        
    YAFTPD_CONFIG_LOOKUP(int, inst_port);
    YAFTPD_CONFIG_LOOKUP(int, data_port);
    YAFTPD_CONFIG_LOOKUP(string, welcome_message);
    YAFTPD_CONFIG_LOOKUP(int, listen_queue_size);
    YAFTPD_CONFIG_LOOKUP(int, inst_buffer_size);
    YAFTPD_CONFIG_LOOKUP(bool, anonymous_login);
    YAFTPD_CONFIG_LOOKUP(int, anonymous_mask);
    YAFTPD_CONFIG_LOOKUP(string, anonymous_root);
    YAFTPD_CONFIG_LOOKUP(int, listing_line_size);
#undef YAFTPD_CONFIG_LOOKUP

    config_destroy(cfg);
    return;
}

static void yaftpd_session(yaftpd_state_t *yaftpd_state)
{
    yaftpd_config_t *yaftpd_config = yaftpd_state->config;
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, yaftpd_config->welcome_message);

    char instbuff[yaftpd_config->inst_buffer_size];
    int inst_msg_len;
    while (inst_msg_len = recv(yaftpd_state->inst_conn_fd, instbuff, yaftpd_config->inst_buffer_size, 0))
    {
        /* assuming the maximum instruction size is no more than inst_buffer_size */
        if (inst_msg_len == yaftpd_config->inst_buffer_size)
            fprintf(stderr, "%s: warning: Maximum instruction size exceeded.", program_invocation_short_name);
        instbuff[inst_msg_len] = 0; /* manually added \0 for c strings */

        /* a naive parsing routine. shall use flex+bison instead. */
        int i;
        const session_response_t *sp;
        for (i = 0; sp = &session_response[i], sp->instname; i++)
            if (!strncasecmp(sp->instname, instbuff, strlen(sp->instname)))
            {
                /* TODO: rewrite this to multi-thread */
                /* shall be handled by a new thread */
                sp->handler(instbuff, yaftpd_state);
                break;
            }
        if (!sp->instname) /* not parsed */
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "502 Command not implemneted.\r\n");
            
        printf("inst_msg len: %d\n", inst_msg_len);
        if (inst_msg_len > 0)
            puts(instbuff);
    }
    return;
}

int main(int argc, char **argv)
{   
    /* to work with chroot jail we need root privilege,
     * also when the port number is less than 1024 it requires root */
    if (chroot("/") < 0)
        err(1, "requires root to work");

    yaftpd_config_t _config = { };
    yaftpd_state_t _yaftpd_state = { .config = &_config }, *yaftpd_state = &_yaftpd_state;
    config_file_read("yaftpd.conf", yaftpd_state);
    if (socket_listen(yaftpd_state, INADDR_ANY, yaftpd_state->config->inst_port, &yaftpd_state->inst_listen_fd))
        exit(1);
    
    /* main loop */
    while (1)
    {
        int saddr_len = sizeof(yaftpd_state->saddr);
        /* this would block until an incoming connection */
        yaftpd_state->inst_conn_fd = accept(yaftpd_state->inst_listen_fd, &yaftpd_state->saddr, &saddr_len);
        if (yaftpd_state->inst_conn_fd < 0)
            err(1, "error accepting connection");
        pid_t pid = fork();
        if (pid == 0)
        {
            /* child process */
            yaftpd_session(yaftpd_state);
            close(yaftpd_state->inst_conn_fd);
            close(yaftpd_state->inst_listen_fd);
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
            warn("error forking");
            close(yaftpd_state->inst_conn_fd);
        }
    }

    /* execution flow shall not reach here */
    err(1, "this line shall not be executed");

    return 0;
}

