/* a toy ftp client in 64 lines by Pengyu CHEN(cpy.prefers.you@gmail.com) */
#include <err.h> // remove me later
#include <stdio.h> // remove me later
#include <sys/socket.h> // remove me later
#include <sys/types.h> // remove me later
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
char bbuf[0x10000],*abuf=bbuf+32, *user, *pass, ap[14],ap2[14],*mode,*fname,*inst;
int instfd,localfd,datafd;
int main(int argc, char **argv){
#define CONN(fd,addr) ({struct sockaddr sa = {AF_INET, {ap[1],ap[0],ap[2],ap[3],ap[4],ap[5]}};\
    connect(fd = socket(AF_INET, SOCK_STREAM, 0), &sa, sizeof(sa));\
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&(struct timeval){0,1<<18},sizeof(struct timeval));})
#define RECV(sockfd,outfd) while(write(outfd,bbuf,recv(sockfd,bbuf,0x10000,0))>0);
    while(write(1,"> ",2,0), gets(abuf)){
        if (sscanf(abuf, "open %hhu.%hhu.%hhu.%hhu:%hu",
            &ap[2],&ap[3],&ap[4],&ap[5],ap)==5) {
            CONN(instfd,ap);RECV(instfd,1);
        }
        if (!strncasecmp(abuf, "local ", 6))
            system(abuf + 6);
        if (!strncasecmp(abuf, "remote ", 7)) {
            *(int*)(abuf+strlen(abuf)) = 0x00000A0D;
            send(instfd, abuf+7, strlen(abuf+7), 0);
            RECV(instfd,1);
        }
        if (sscanf(abuf, "setpasv %hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
            ap2+2,ap2+3,ap2+4,ap2+5,ap2+1,ap2) == 6) puts("Succeeded.");
        if (sscanf(abuf, "remote-get %m[^\n]",&fname) == 1){
            asprintf(&inst,"RETR %s\r\n",fname);
            send(instfd, inst, strlen(inst), 0);
            CONN(datafd,ap2);
            localfd=open(fname,O_WRONLY|O_CREAT,0777);
            RECV(datafd,localfd);
            close(localfd);
            RECV(instfd,1);
        }
        if (sscanf(abuf, "remote-put %m[^\n]",&fname) == 1){
            asprintf(&inst,"STOR %s\r\n",fname);
            send(instfd,inst,strlen(inst),0);
            CONN(datafd,ap2);
            localfd=open(fname,O_RDONLY);
            while(send(datafd,bbuf,read(datafd,bbuf,0x10000,0),0)>0);
            close(localfd);
            RECV(instfd,1);
        }
        if (sscanf(abuf,"remote-disp %m[^\n]", &inst) == 1)  {
            send(instfd, inst, strlen(inst), 0);
            CONN(datafd,ap2);
            RECV(datafd,1);
            RECV(instfd,1);
        }
    }
    puts("why");
    return;
}
