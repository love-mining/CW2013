/* a toy ftp client in 64 lines by Pengyu CHEN(cpy.prefers.you@gmail.com) */
#include <err.h> // remove me later
#include <stdio.h> // remove me later
#include <sys/socket.h> // remove me later
#include <sys/types.h> // remove me later
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
char bbuf[0x10000],ap[14],ap2[14],*fname,*inst;
int instfd,localfd,datafd,tmp;
int main(int argc, char **argv){
#define CONN(fd,ap) ({struct sockaddr sa = {AF_INET, {ap[1],ap[0],ap[2],ap[3],ap[4],ap[5]}};\
  connect(fd = socket(AF_INET, SOCK_STREAM, 0), &sa, sizeof(sa));\
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&(struct timeval){0,1<<18},sizeof(struct timeval));})
#define RECV(sockfd,outfd) ({tmp=outfd;while(write(tmp,bbuf,recv(sockfd,bbuf,0x10000,0))>0);})
#define SEND(fd,msg) send(fd,msg,strlen(msg),0)
#define SENDRN(fd,msg) ({SEND(fd,msg);SEND(fd,"\r\n");})
  while(write(1,"ftp64> ",7,0), gets(bbuf)){
    if(sscanf(bbuf,"open %hhu.%hhu.%hhu.%hhu:%hu",ap+2,ap+3,ap+4,ap+5,ap)==5){
      CONN(instfd,ap);RECV(instfd,1);
    } else if (!strncasecmp(bbuf, "local ", 6)) { system(bbuf + 6);
    } else if (!strncasecmp(bbuf, "remote ", 7)) { SENDRN(instfd,bbuf+7); RECV(instfd,1);
    } else if (sscanf(bbuf, "setpasv %hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
      ap2+2,ap2+3,ap2+4,ap2+5,ap2+1,ap2) == 6) { puts("succeeded.");
    } else if (sscanf(bbuf, "remote-get %m[^\n]",&fname) == 1){
      SEND(instfd,"RETR "); SENDRN(instfd,fname); CONN(datafd,ap2);
      localfd=open(fname,O_WRONLY|O_CREAT,0755);RECV(datafd,localfd);
      close(localfd); RECV(instfd,1);
    } else if (sscanf(bbuf, "remote-put %m[^\n]",&fname) == 1){
      SEND(instfd,"STOR "); SENDRN(instfd,fname); CONN(datafd,ap2);localfd=open(fname,O_RDONLY);
      while(send(datafd,bbuf,read(localfd,bbuf,0x10000,0),0)>0);close(localfd); RECV(instfd,1);
    } else if (sscanf(bbuf,"remote-disp %m[^\n]", &inst) == 1)  {
      SENDRN(instfd,inst); CONN(datafd,ap2); RECV(datafd,1); RECV(instfd,1); 
    } else puts("syntax error.");
  }
  return;
}
