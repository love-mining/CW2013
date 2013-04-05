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
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libconfig.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    int listing_line_size;
    int file_read_buffer_size;
    int data_listener_retry;
    int use_chroot_jail;
}   yaftpd_config_t;

typedef struct _yaftpd_state_t
{
    yaftpd_config_t *config;
    int inst_listen_fd;
    int inst_conn_fd;
    int data_conn_mode;
    int data_conn_addr;
    int data_conn_port;
    int data_listen_fd;
    int data_conn_fd;
    struct sockaddr saddr;
    const char *username;
    struct passwd *pw;
    struct group *gr;
    int loggedin;
    int quit;
    size_t rest_offset;
}   yaftpd_state_t;

/* SIGCHLD shall be handled or terminated child processes would remain zombie */
static void sig_chld_hndl(int signo)
{
    int status;
    if (wait(&status) < 0)
        warn("error waiting for child process");
    return;
}

static inline char ftypelet(mode_t bits)
{
    if (S_ISREG (bits))
        return '-';
    if (S_ISDIR (bits))
        return 'd';
    if (S_ISBLK (bits))
        return 'b';
    if (S_ISCHR (bits))
        return 'c';
    if (S_ISLNK (bits))
        return 'l';
    if (S_ISFIFO (bits))
        return 'p';
    if (S_ISSOCK (bits))
        return 's';
    return '?';
}

static void filemodestring(mode_t mode, char *str)
{
    str[0] = ftypelet (mode);
    str[1] = mode & S_IRUSR ? 'r' : '-';
    str[2] = mode & S_IWUSR ? 'w' : '-';
    str[3] = (mode & S_ISUID
        ? (mode & S_IXUSR ? 's' : 'S')
        : (mode & S_IXUSR ? 'x' : '-'));
    str[4] = mode & S_IRGRP ? 'r' : '-';
    str[5] = mode & S_IWGRP ? 'w' : '-';
    str[6] = (mode & S_ISGID
        ? (mode & S_IXGRP ? 's' : 'S')
        : (mode & S_IXGRP ? 'x' : '-'));
    str[7] = mode & S_IROTH ? 'r' : '-';
    str[8] = mode & S_IWOTH ? 'w' : '-';
    str[9] = (mode & S_ISVTX
        ? (mode & S_IXOTH ? 't' : 'T')
        : (mode & S_IXOTH ? 'x' : '-'));
    str[10] = ' ';
    str[11] = '\0';
    return;
}    

static void pw_gr_init(yaftpd_state_t *yaftpd_state)
{
    int i;
    
    FILE *pwin = fopen("/etc/passwd", "r");
    int pwlcnt = 0;
    while (fscanf(pwin, "%*[^\n]\n") != EOF)
        pwlcnt += 1;
    struct passwd *pw = malloc((pwlcnt + 1) * sizeof(struct passwd));
    yaftpd_state->pw = pw;
    fseek(pwin, 0, SEEK_SET);
    for (i = 0; i < pwlcnt; i++)
        fscanf(pwin, 
            "%m[^:]:%m[^:]:%d:%d:%m[^:]:%m[^:]:%m[^:\n]\n",
            &pw[i].pw_name,
            &pw[i].pw_passwd,
            &pw[i].pw_uid,
            &pw[i].pw_gid,
            &pw[i].pw_gecos,
            &pw[i].pw_dir,
            &pw[i].pw_shell
        );
    pw[pwlcnt].pw_uid = -1; /* tail sentinal */
    fclose(pwin);

    FILE *grin = fopen("/etc/group", "r");
    int grlcnt = 0;
    while (fscanf(grin, "%*[^\n]\n") != EOF)
        grlcnt += 1;
    struct group *gr = malloc((grlcnt + 1) * sizeof(struct group));
    yaftpd_state->gr = gr;
    fseek(grin, 0, SEEK_SET);
    char *_tmpstr;
    for (i = 0; i < grlcnt; i++)
    {
        fscanf(grin, 
            "%m[^:]:%m[^:]:%d%m[^\n]\n",
            &gr[i].gr_name,
            &gr[i].gr_passwd,
            &gr[i].gr_gid,
            &_tmpstr /* starts with a colon. deals with empty group members */
        );
        char *tmpstr = _tmpstr + 1;
        int memcnt = *tmpstr ? 1 : 0;
        while (*tmpstr)
        {
            if (*tmpstr == ',')
                memcnt += 1;
            tmpstr++;
        }
        char **mem = malloc((memcnt + 1) * sizeof(char*));
        gr[i].gr_mem = mem;
        mem[0] = strtok(tmpstr, ",");
        mem += 1;
        while (*mem = strtok(NULL, ","))
            mem += 1;
    }
    gr[grlcnt].gr_gid = -1; /* tail sentinal */
    fclose(grin);
    return;
}

static const struct passwd *getpw_uid(yaftpd_state_t *yaftpd_state, uid_t uid)
{
    int i;
    struct passwd *pw = yaftpd_state->pw;
    for (i = 0; pw[i].pw_uid != -1; i++)
        if (pw[i].pw_uid == uid)
            return &pw[i];
    
    static const struct passwd unknown = {
        .pw_name = "unknown",
        .pw_passwd = "x",
        .pw_uid = -1,
        .pw_gid = -1,
        .pw_gecos = "unknown",
        .pw_dir = "unknown",
        .pw_shell = "unknown",
    };
    return &unknown;
}

static const struct group *getgr_gid(yaftpd_state_t *yaftpd_state, gid_t gid)
{
    int i;
    struct group *gr = yaftpd_state->gr;
    for (i = 0; gr[i].gr_gid != -1; i++)
        if (gr[i].gr_gid == gid)
            return &gr[i];
    
    static const struct group unknown = {
        .gr_name = "unknown",
        .gr_passwd = "x",
        .gr_gid = -1,
        .gr_mem = NULL,
    };
    return &unknown;
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
        warn("error binding socket, retrying");
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
    if (send(conn_fd, msg, msglen, 0) == -1)
        warn("error sending formatted message to socket");
    free(msg);
    return;
}

static int yaftpd_setup_data_conn(yaftpd_state_t *yaftpd_state)
{
    /* preparing the data connection */
    if (yaftpd_state->data_conn_mode == YAFTPD_ACTIVE)
    {
        int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (conn_fd < 0)
        {
            warn("error creating socket");
            return -1;
        }
        char *addrstr;
        asprintf(&addrstr, "%u", yaftpd_state->data_conn_addr);
        struct sockaddr_in saddrin = {
            .sin_family = AF_INET,
            .sin_port = htons(yaftpd_state->data_conn_port),
            .sin_addr = inet_addr(addrstr),
        };
        if (connect(conn_fd, (struct sockaddr*)&saddrin, sizeof(saddrin)) == -1)
        {
            warn("error connecting socket");
            return -1;
        }
        free(addrstr);
        yaftpd_state->data_conn_fd = conn_fd;
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "150 Opening data connection.\r\n");
    }
    else if (yaftpd_state->data_conn_mode == YAFTPD_PASSIVE)
    {
        if (yaftpd_state->data_listen_fd < 0)
            return -1;
        struct sockaddr dsaddr;        
        int saddr_len = sizeof(dsaddr);
        
        /* this would block until an incoming connection */
        yaftpd_state->data_conn_fd = accept(yaftpd_state->data_listen_fd, &dsaddr, &saddr_len);
        if (yaftpd_state->data_conn_fd < 0)
            return -1;
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "150 Opening data connection.\r\n");
    }
    else
        return -1;
    return 0;
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
    if (sscanf(instruction, "USER %[^\r\n]", username) < 1)
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
    if (sscanf(instruction, "PASS %[^\r]", password) < 1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
    else
    {
        if (!yaftpd_state->username)
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "503 Login with USER first.\r\n");
        else if (yaftpd_password_validate(password, yaftpd_state))
        {
            const char *username = yaftpd_state->username;
            if (!strcasecmp(yaftpd_state->username, "anonymous"))
                username = "ftp";
            struct passwd *pw = getpwnam(username);
            if (pw)
            {
                chdir(pw->pw_dir);
                if (yaftpd_state->config->use_chroot_jail)
                    chroot(pw->pw_dir);
                if (setegid(pw->pw_gid) || seteuid(pw->pw_uid)) /* the order here matters */
                    warn("error setting euid and egid");
            }
            else
                warn("failed to get struct passwd");
            yaftpd_state->loggedin = 1;
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
    case 'A':   /* ASCII files */
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

    yaftpd_state->data_conn_mode = YAFTPD_PASSIVE;
    int i;
    for (i = 0; i < yaftpd_state->config->data_listener_retry; i++)
    {
        yaftpd_state->config->data_port = rand() % 32768 + 1024;
        if (socket_listen(yaftpd_state, INADDR_ANY, yaftpd_state->config->data_port, &yaftpd_state->data_listen_fd) >= 0)
            break;
    }
    if (i == yaftpd_state->config->data_listener_retry)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open passive connection.\r\n");
        return;
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
    int port = yaftpd_state->config->data_port; 
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n",
        iaddr[0], iaddr[1], iaddr[2], iaddr[3], port >> 8, port & 255);
    return;
}

static void session_response_list(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "LIST %[^\r]", param) < 1)
        target = ".";

    if (yaftpd_setup_data_conn(yaftpd_state) < 0)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
        return;
    }

    /* do the listing and send data */
    /* assuming the target is always a directory */
    DIR *dir = opendir(target);
    if (dir)
    {
        struct dirent *dent;
        while ((dent = readdir(dir)) != NULL)
        {
            struct stat statbuf;
            char fpath[yaftpd_state->config->inst_buffer_size];
            sprintf(fpath, "%s/%s", target, dent->d_name);
            if (stat(fpath, &statbuf) == -1)
            {
                warn("failed to stat: %s", dent->d_name);
                break;
            }
            char modestr[16];
            filemodestring(statbuf.st_mode, modestr);

            const struct passwd *pw = getpw_uid(yaftpd_state, statbuf.st_uid);
            if (!pw)
            {
                warn("failed to getpwuid %d", statbuf.st_uid);
                break;
            }
            char *username = strdup(pw->pw_name);

            const struct group *gr = getgr_gid(yaftpd_state, statbuf.st_gid);
            if (!gr)
            {
                warn("failed to getgrgid %d", statbuf.st_gid);
                break;
            }
            char *groupname = strdup(gr->gr_name);

            char *mtime = strdup(ctime(&statbuf.st_mtime));
            /* manually stripping the last new line */
            mtime[strlen(mtime) - 1] = 0;
 
            int mtimestr_sz = 16;
            char mtimestr[mtimestr_sz];
            strftime(mtimestr, mtimestr_sz, "%b %e  %Y", localtime(&statbuf.st_mtime));
            
            socket_send_fmtstr(yaftpd_state->data_conn_fd, 
                "%s %d %s %s %lld %s %s\r\n",
                modestr,
                statbuf.st_nlink,
                username,
                groupname,
                (unsigned long long)statbuf.st_size,
                mtimestr,
                dent->d_name
                );
            free(username);
            free(groupname);
        }
        if (!dent)
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "226 Listing complete.\r\n");
        else
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "551 File listing failed.\r\n");
        closedir(dir);
    }
    else
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "551 File listing failed: %s.\r\n", strerror(errno));
    }
    
    close(yaftpd_state->data_conn_fd);
    return;
}

static void session_response_cwd(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "CWD %[^\r]", param) < 1)
        target = "/";
    if (chdir(target))
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 No such directory.\r\n");
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "250 Command okay.\r\n");

    return;
}

static void session_response_retr(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "RETR %[^\r]", param) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }
    int retr_fd = open(param, O_RDONLY);
    if (retr_fd == -1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
        return;
    }
    if (lseek(retr_fd, yaftpd_state->rest_offset, SEEK_SET) == -1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
        return;
    }
    yaftpd_state->rest_offset = 0;
    if (yaftpd_setup_data_conn(yaftpd_state) < 0)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
        return;
    }

    int buffsz = yaftpd_state->config->file_read_buffer_size;
    char buf[buffsz];
    int readsz;
    int sendret;
    while ((readsz = read(retr_fd, buf, buffsz)) > 0)
        if ((sendret = send(yaftpd_state->data_conn_fd, buf, readsz, 0)) == -1)
            break;
    if (readsz == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "426 %s.\r\n", strerror(errno));
    else if (sendret == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "551 %s.\r\n", strerror(errno));
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "226 Transfer complete.\r\n");
    close(yaftpd_state->data_conn_fd);
    close(retr_fd);
    return;
}

static void session_response_stor(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "STOR %[^\r]", param) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }

    int stor_fd = open(param, O_WRONLY | O_CREAT, 0777);
    if (stor_fd == -1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
        return;
    }
    if (lseek(stor_fd, yaftpd_state->rest_offset, SEEK_SET) == -1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
        return;
    }
    yaftpd_state->rest_offset = 0;

    if (yaftpd_setup_data_conn(yaftpd_state) < 0)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
        return;
    }

    int buffsz = yaftpd_state->config->file_read_buffer_size;
    char buf[buffsz];
    int readsz;
    int writeret;
    while ((readsz = recv(yaftpd_state->data_conn_fd, buf, buffsz, 0)) > 0)
        if ((writeret = write(stor_fd, buf, readsz)) == -1)
            break;
    if (readsz == -1 && errno != ECONNRESET)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "426 %s.\r\n", strerror(errno));
    else if (writeret == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "551 %s.\r\n", strerror(errno));
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "226 Transfer complete.\r\n");
    close(yaftpd_state->data_conn_fd);
    close(stor_fd);
    return;
}

static void session_response_feat(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    const char *featstr = 
        "211-Features\r\n"
        "211 End\r\n";
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, featstr);
    return;
}

static void session_response_syst(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "215 UNIX: GLaDOS\r\n");
    return;
}

static void session_response_dele(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    if (sscanf(instruction, "DELE %[^\r]", param) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }
    if (unlink(param) == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "450 %s.\r\n", strerror(errno));
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "250 Command okay.\r\n");
    return;
}

static void session_response_mkd(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    if (sscanf(instruction, "MKD %[^\r]", param) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }
    if (mkdir(param, 0777) == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "250 Directory created.\r\n");
    return;
}

static void session_response_rmd(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    if (sscanf(instruction, "RMD %[^\r]", param) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }
    if (rmdir(param) == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "250 Directory created.\r\n");
    return;
}

static void session_response_quit(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    yaftpd_state->quit = 1;
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "221 Goodbye, and, have a nice day. :)\r\n");
    return;
}

static void session_response_port(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    int addr[4], port[2];
    if (sscanf(instruction, "PORT %d,%d,%d,%d,%d,%d", 
        &addr[0], &addr[1], &addr[2], &addr[3], &port[0], &port[1]) != 6)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }

    yaftpd_state->data_conn_mode = YAFTPD_ACTIVE;
    yaftpd_state->data_conn_addr = (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3];
    yaftpd_state->data_conn_port = (port[0] << 8) | port[1];
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "220 PORT okay.\r\n");
    return;
}

static void session_response_appe(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    char param[yaftpd_state->config->inst_buffer_size];
    char *target = param;
    if (sscanf(instruction, "APPE %[^\r]", param) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }

    int stor_fd = open(param, O_WRONLY | O_APPEND);
    if (stor_fd == -1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "550 %s.\r\n", strerror(errno));
        return;
    }
    if (yaftpd_setup_data_conn(yaftpd_state) < 0)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "425 Cannot open data connection.\r\n");
        return;
    }

    int buffsz = yaftpd_state->config->file_read_buffer_size;
    char buf[buffsz];
    int readsz;
    int writeret;
    while ((readsz = recv(yaftpd_state->data_conn_fd, buf, buffsz, 0)) > 0)
        if ((writeret = write(stor_fd, buf, readsz)) == -1)
            break;
    if (readsz == -1 && errno != ECONNRESET)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "426 %s.\r\n", strerror(errno));
    else if (writeret == -1)
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "551 %s.\r\n", strerror(errno));
    else
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "226 Transfer complete.\r\n");
    close(yaftpd_state->data_conn_fd);
    close(stor_fd);
    return;
}

static void session_response_rest(const char *instruction, yaftpd_state_t *yaftpd_state)
{
    unsigned long long offset;
    if (sscanf(instruction, "REST %llu", &offset) < 1)
    {
        socket_send_fmtstr(yaftpd_state->inst_conn_fd, "501 Syntax error.\r\n");
        return;
    }

    yaftpd_state->rest_offset = offset;
    socket_send_fmtstr(yaftpd_state->inst_conn_fd, "350 Offset set at %llu.\r\n", offset);
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
    { "LIST",   session_response_list },
    { "CWD",    session_response_cwd },
    { "RETR",   session_response_retr },
    { "STOR",   session_response_stor },
    { "FEAT",   session_response_feat },
    { "SYST",   session_response_syst },
    { "DELE",   session_response_dele },
    { "MKD",    session_response_mkd },
    { "RMD",    session_response_rmd },
    { "QUIT",   session_response_quit },
    { "PORT",   session_response_port },
    { "APPE",   session_response_appe },
    { "REST",   session_response_rest },
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
    YAFTPD_CONFIG_LOOKUP(int, listing_line_size);
    YAFTPD_CONFIG_LOOKUP(int, file_read_buffer_size);
    YAFTPD_CONFIG_LOOKUP(int, data_listener_retry);
    YAFTPD_CONFIG_LOOKUP(bool, use_chroot_jail);
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
        printf("PID=%d: Command received: %s", getpid(), instbuff);

        /* a naive parsing routine. shall use flex+bison instead. */
        int i;
        const session_response_t *sp;
        for (i = 0; sp = &session_response[i], sp->instname; i++)
            if (!strncasecmp(sp->instname, instbuff, strlen(sp->instname)))
               break;
        if (!sp->instname) /* not parsed */
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "502 Command not implemneted.\r\n");
        else if (strncasecmp(instbuff, "USER", strlen("USER"))
            && strncasecmp(instbuff, "PASS", strlen("PASS"))
            && !yaftpd_state->loggedin)
            socket_send_fmtstr(yaftpd_state->inst_conn_fd, "503 Not logged in.\r\n");
        else
        {
            /* TODO: rewrite this to multi-thread */
            /* shall be handled by a new thread */
            sp->handler(instbuff, yaftpd_state);
        }
        printf("PID=%d: Command finished: %s", getpid(), instbuff);
        if (yaftpd_state->quit)
            break;
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
    yaftpd_state_t _yaftpd_state = {
        .config = &_config,
        //.loggedin = 0,
        //.quit = 0,
        //.rest_offset = 0,
    };
    yaftpd_state_t *yaftpd_state = &_yaftpd_state;
    config_file_read("yaftpd.conf", yaftpd_state);
    pw_gr_init(yaftpd_state);
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

