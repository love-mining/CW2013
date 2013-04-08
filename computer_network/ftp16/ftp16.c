*s[]={"\r\n","RETR ","STOR ","ftp> ","succeeded","syntax error","local%*[ ]%m["
"^\n]","remote%*[ ]%m[^\n]","remote-get%*[ ]%m[^\n]","remote-put%*[ ]%m[^\n]",
"remote-disp%*[ ]%m[^\n]","open%*[ ]%hhu.%hhu.%hhu.%hhu:%hu","setpasv%*[ ]%hhu"
",%hhu,%hhu,%hhu,%hhu,%hhu","a toy ftp client by Pengyu CHEN\nlist of commands"
":\nopen addr.addr.addr.addr:port\nlocal command\nremote command\nremote-get f"
"ile\nremote-put file\nremote-disp command\nsetpasv addr,addr,addr,addr,port,p"
"ort"},*f,*c,t;char _[1<<16],*a=_+100;S(f,m){send(f,m,strlen(m),0);}main(i,l,d)
#define E(n) else if(sscanf(_,s[n],
{puts(s[13]);while(write(1,s[3],5,0),gets(_)){if(0);E(11)a+2,a+3,a+4,a+5,a)==5)
C(&i),R(i,1);E(6)&c)==1)system(c);E(7)&c)==1)S(i,c),S(i,*s),R(i,1);E(12)a+2,a+3
,a+4,a+5,a+1,a)==6)puts(s[4]);E(8)&f)==1)S(i,s[1]),S(i,f),S(i,*s),C(&d),R(d,l=
open(f,65,493)),close(l),R(i,1);E(9)&f)==1)l=open(f,0),S(i,s[2]),S(i,f),S(i,*s)
,C(&d),({while(send(d,_,read(l,_,1<<16,0),0)>0);}),close(l),R(i,1);E(10)&c)==1)
S(i,c),S(i,*s),C(&d),R(d,1),R(i,1);else puts(s[5]);}}R(s,o){while((t=recv(s,_,1
<<16,0))>0&&write(o,_,t)>0);}C(int*f){char t[]={2,0,a[1],a[0],a[2],a[3],a[4],a[
5]};connect(*f=socket(2,1,0),t,16);setsockopt(*f,1,20,&(int[]){0,1<<19},16);}
