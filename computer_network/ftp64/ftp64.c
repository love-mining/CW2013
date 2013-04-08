*s[]={"\r\n","RETR ","STOR ","local ","remote ","ftp32> ","succeeded",
"syntax error","remote-get %m[^\n]","remote-put %m[^\n]","remote-disp %m[^\n]"
,"open %hhu.%hhu.%hhu.%hhu:%hu","setpasv %hhu,%hhu,%hhu,%hhu,%hhu,%hhu"},
*f,*c;char _[0x10000],*a=_+100;main(i,l,d,t,x){
#define CONN(f,a)({char t[]={2,0,a[1],a[0],a[2],a[3],a[4],a[5]};\
  connect(f=socket(2,1,0),t,16);setsockopt(f,1,20,&(int[]){0,1<<19},16);})
#define RECV(s,o)({t=o;while((x=recv(s,_,0x10000,0))>0&&write(t,_,x)>0);})
#define SEND(f,m) ({send(f,m,strlen(m),0);})
#define SENDRN(f,m) ({SEND(f,m);SEND(f,s[0]);})
  while(write(1,s[5],7,0),gets(_)){
    if(sscanf(_,s[11],a+2,a+3,a+4,a+5,a)==5)CONN(i,a),RECV(i,1);
    else if (!strncasecmp(_,s[3],6))system(_ + 6);
    else if (!strncasecmp(_,s[4],7))SENDRN(i,_+7),RECV(i,1);
    else if (sscanf(_,s[12],a+2,a+3,a+4,a+5,a+1,a)==6)puts(s[6]);
    else if (sscanf(_,s[8],&f)==1)SEND(i,*s),SENDRN(i,f),CONN(d,a),
      l=open(f,1|0100,0755),RECV(d,l),close(l),RECV(i,1);
    else if (sscanf(_,s[9],&f)==1)SEND(i,s[2]),SENDRN(i,f),CONN(d,a),
      l=open(f,0),({while(send(d,_,read(l,_,0x10000,0),0)>0);}),
      close(l),RECV(i,1);
    else if (sscanf(_,s[10],&c)==1)SENDRN(i,c),CONN(d,a),RECV(d,1),RECV(i,1); 
    else puts(s[7]);
}return;}
