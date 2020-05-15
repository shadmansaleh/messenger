#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

int client_fd=0;
int toId=-1;
int userId=-1;
char name[20];
char to[20]="Not-set";
//char oldname[20];

void error(char *er);
int read_in(int fd,char *data,int dsize);
int say(int socket, char *m); 
char * messageCrafter(char *from,char *to,char *m);
void getto(char *usrto,char *msg);
void getfrom(char *usrfrom,char *msg);
void getmessage(char *message,char *msg);
void interpreteMessage(char *userfrom,char *userto,char *message,char *msg);
void handleMessage(char *msg);
void handle_shutdown(int sig);
int handleInput(char *in);
int _send(int fd,char *msg);
int _recv(int fd,char *msg,int size);


int main(int argc,char *argv[]){
	int port=-1;
	if(argc!=3||((port=atoi(argv[2]))==0)){
		fprintf(stderr,"%s [server ip] [port]\n",argv[0]);
		return 1;
	}

	port=atoi(argv[2]);

	struct sockaddr_in clientaddr;
	clientaddr.sin_family=AF_INET;
	clientaddr.sin_addr.s_addr=inet_addr(argv[1]);
	clientaddr.sin_port=htons(port);

	int result;
	char buffer[1025];

	struct sigaction term;
	term.sa_handler=handle_shutdown;
	sigemptyset(&term.sa_mask);
	term.sa_flags=0;
	sigaction(SIGINT,&term,NULL);


//	unsigned int addrlen=sizeof(clientaddr);

	if((client_fd=socket(AF_INET,SOCK_STREAM,0))==-1)
		error("Faild to create socket");
	if(connect(client_fd,(struct sockaddr*)&clientaddr,sizeof(clientaddr))==-1)
		error("Connection failed");
	printf("Waiting for servers reply\n");
	result=read_in(client_fd,buffer,1025);
	if(result==0 || result==-1)
		error("Connection failed");
	if(strstr(buffer,"Server full")){
		handleMessage(buffer);
		shutdown(client_fd,SHUT_RDWR);
		close(client_fd);
		exit(0);
	}
	else{
		char *a;
		char *i;
		i=strstr(buffer,"<id:")+sizeof("<id:")-1;
		a=strstr(buffer,"<username:")+sizeof("<username:")-1;
		*(strchr(i,'>'))='\0';
		*(strchr(a,'>'))='\0';
		userId=atoi(i);
//		printf("id=%d\n",userId);
		memset(name,'\0',20);
		memmove(name,a,strlen(a));
//		printf("Connection accepted by server.type @help to see all commands");
	}

	char usrname[20];
get_username:
	memset(usrname,'\0',sizeof(usrname));
	printf("\nEnter username : ");
	scanf("%19s",usrname);
	memset(buffer,'\0',1025);
	sprintf(buffer,"@set-name %s",usrname);
	if(handleInput(buffer)==0)
//	memset(name,'\0',20);
//	memmove(name,usrname,strlen(usrname));
		printf("Connection accepted by server.type @help to see all commands\n");
	else
		goto get_username;
	fd_set readfds;
	printf("To is not set . please set to with @set-to command before sending message\n\n");

	while(1){
//		memset(oldname,'\0',20);
//		memmove(oldname,name,sizeof(name));
//		puts("inside loop");
//		printf(">To:%s> ",to);
		FD_ZERO(&readfds);
		FD_SET(client_fd,&readfds);
		FD_SET(0,&readfds);
		result=select(client_fd+1,&readfds,NULL,NULL,NULL);
		if(result==-1){
			perror("select exited");
			continue;
		}
		else if(FD_ISSET(0,&readfds)){
			//taking user input
//			puts("\nTAKING USER INPUT\n");
			memset(buffer,'\0',1025);
			fgets(buffer,1000,stdin);
			*(strchr(buffer,'\n'))='\0';
			handleInput(buffer);
		//	printf(">To:%s> ",to);
		}
		else if(FD_ISSET(client_fd,&readfds)){
			//receiving message
//			puts("\nRECEIVING SERVER DATA\n");
			memset(buffer,'\0',1025);
			result=read_in(client_fd,buffer,1025);
	//		result=(recv(client_fd,buffer,1025,0));
//			printf("%s\n",buffer);
			if(result==0 || result==-1){
				shutdown(client_fd,SHUT_RDWR);
				close(client_fd);
				perror("Connection failed");
				exit(0);
			}else{
				handleMessage(buffer);
//				printf(">To:%s> ",to);
			}

		}
	}


}

void error(char *er){
	fprintf(stderr,"%s : %s\n",er,strerror(errno));
	exit(1);
}

/*int read_in(int fd,char *data,int dsize){   
	int res=recv(fd,data,dsize,0);
	return res;
//	int i=0;
        char chr;
        size_t s=0;   
	while (i<dsize){   
		s=read(fd,&chr,1);
		if(chr=='!'||chr=='\0' || s==-1 || s==0){  
			      break;
                }
                data[i]=chr;
                i++;  
	} 
	if(i+1<dsize)
		data[i+1]='\0';
	else
		data[i]='\0';

//	printf("received %s\n",data);


       if(s==-1)  
	       return -1;
       else
//               return i;  
}*/

/*int say(int socket, char *m) {
	int res=send(socket,m,strlen(m),0);
	return res;
//	char *s=m;
	int siz=strlen(s);
	int r=0;
	int result=0;
	int retry=0;
resay:
//	printf ("trieng to forword %s to socket %d\n",m,socket);
	result = write(socket, s, sizeof(char)*siz); 
	if (result == -1) {  
 		fprintf(stderr, "%s: %s\n", "Error talking to the client", strerror(errno));
		if(retry <6){
 			retry++;
			goto resay;
		}
		else {
			perror("server disconnected\n");
			shutdown(client_fd,SHUT_RDWR);
			close(client_fd);
			return -1;
		}
	}
	else if(result/sizeof(char)<siz){
 		r+=result;
		m+=result/sizeof(char);
		siz-=result/sizeof(char);
		goto resay;
	}
	r+=result;
//	printf("sent %s to socket %d",m,socket);
//	puts("sent");
//	return r; 
}*/

int read_in(int fd,char *data,int dsize){
	memset(data,'\0',dsize);
	int res;
	int len;
	char buff[10];
	char *d=data;
	//get size
	int retry=0;
get_size:
	memset(buff,'\0',10);
	res=recv(fd,buff,10,0);
	if(res==0)
		return 0;
//		removeuser(-1,fd,NULL);
	else if(res==-1&&retry <=5){
		retry++;
		goto get_size;
	}
	else{
		retry=0;
		if(buff[strlen(buff)-1]==':'){
			buff[strlen(buff)-1]='\0';
			len=atoi(buff);
			if(len!=0)
				_send(fd,"size received");
			else{
				_send(fd,"send size");
				goto get_size;
			}
		}else{
			_send(fd,"send size");
			goto get_size;
		}
	}
	res=0;
get_msg:
	res=_recv(fd,d,len);
	if(res==0)
		return 0;
//		removeuser(-1,fd,NULL);
	else if(res==len)
		_send(fd,"message received");
	else{
		_send(fd,"send message");
		goto get_msg;
	}
	return res;
}

int _recv(int fd,char *msg,int size){
	int len=size;
	int res=0;
	char *m=msg;
	int retry=0;
	while(len > 0){
recv_again:
		memset(m,'\0',(len));
		res=recv(fd,m,len,0);
		if(res<=0&&retry<=5){
			retry++;
			goto recv_again;
		}else if(res<=0&&retry>5)
			return 0;
		else{
			retry=0;
			len-=res;
			m+=res;
		}
	}
	return (size-len);
}


int _send(int fd,char *msg){
	int size = strlen(msg);
	int len=size;
	int res=0;
	char *m=msg;
	int retry=0;
	while(len >0){
send_again:
		res=send(fd,m,strlen(m),0);
		if(res<=0 && retry<=5){
			retry++;
			goto send_again;
		}else if(res<= 0 && retry>5)
			return 0;
		else{
			retry=0;
			len-=res;
			m+=res;
		}
	}
	return (size-len);
}

int say(int socket,char *m){
	int res=0;
	int len =strlen(m);
	char buff[100];
	sprintf(buff,"%d:",len);
send_size:
	res=_send(socket,buff);
	if(res==0)
		return 0;
//		removeuser(-1,socket,NULL);
	else{
		memset(buff,'\0',100);
		recv(socket,buff,100,0);
		if(strstr(buff,"send size"))
			goto send_size;
	}
	res=0;
send_msg:
	res=_send(socket,m);
	if(res==0)
		return 0;
//		removeuser(-1,socket,NULL);
	else{
		memset(buff,'\0',100);
		recv(socket,buff,100,0);
		if(strstr(buff,"send message"))
			goto send_msg;
	}
	return res;

}



char * messageCrafter(char *from,char *to,char *m){
	if(strstr(m,"<to:")&&strstr(m,"<from:")&&strstr(m,"<msg:")){
		m=strstr(m,"<msg:")+5;
	}
	char *msg=malloc((strlen(from)+strlen(to)+strlen(m)+30));
	memset(msg,'\0',strlen(from)+strlen(to)+strlen(m)+30);
//	char *msg=malloc(1045);
	sprintf(msg,"<to:%s><from:%s><msg:%s>!",to,from,m);
	return msg;
}

void interpretemessage(char *userfrom,char *userto,char *message,char *msg){
/*	char *tmp=msg;
	userfrom=strstr(tmp,"<from:")+6;
	tmp=strchr(tmp,'>')+1;
	*(tmp-1)='\0';
	userto=strstr(tmp,"<to:")+4;
	tmp=strchr(tmp,'>')+1;
	*(tmp-1)='\0';
	message=strstr(tmp,"<msg:")+5;
	tmp=strchr(tmp,'>');
	*(tmp)='\0';*/
	getfrom(userfrom,msg);
	getto(userto,msg);
	getmessage(message,msg);
}


void getto(char *usrto,char *msg){
	char *tmp=strstr(msg,"<to:")+4;
	char *u=usrto;
	while(*tmp!='>'){
		*u=*tmp;
		u++;
		tmp++;
	}
	*u='\0';
//	printf("To is %s\n",usrto);
	return;
}


void getfrom(char *usrfrom,char *msg){
	char *tmp=strstr(msg,"<from:")+6;
	char *u=usrfrom;
	while(*tmp!='>'){
		*u=*tmp;
		u++;
		tmp++;
	}
	*u='\0';
//	printf("From is %s\n",usrto);
	return;
}
void getmessage(char *message,char *msg){
	char *tmp=strstr(msg,"<msg:")+5;
	char *u=message;
	while(*tmp!='>'){
		*u=*tmp;
		u++;
		tmp++;
	}
	*u='\0';
//	printf("To is %s\n",usrto);
	return;
}

void handle_shutdown(int sig) { 
	if (client_fd)  {
		shutdown(client_fd,SHUT_RDWR);
		close(client_fd);
	}
	fprintf(stderr, "\nBye!\n"); 
	exit(0);
}

void handleMessage(char *msg){
	char from[20];
	char to[20];
	char message[1000];
	interpretemessage(from,to,message,msg);
//	printf("\n\n\t\t\t%s",message);
	if(!strcmp(from,"server"))
		printf("%s\n\t\t(serverReply)\n\n",message);
	else{
		printf("\n\n\t\t\t%s",message);
		printf("\t\t(from:%s)\n\n",from);
	}
}

int handleInput(char *in){
	char *msg;
	if(!strcmp(to,"") && *in!='@'){
	//	printf("To is not set . please set to with @set-to command before sending message\n");
		return -1;
	}
	else if(*in!='@'&&strcmp(to,"Not-set")){
			msg=messageCrafter(name,to,in);
	}
	else{
		if(strstr(in,"@set-to")){
			if(strlen(in+8)<20){
				memset(to,'\0',sizeof(to));
				memmove(to,in+8,strlen(in+8));
				printf("To is set to %s\n",to);
			}
			else
				printf("name is too big\n");
				return -1;
		}
		else if(strstr(in,"@set-name")){
//			char newname[20];
//			memset(newname,'\0',20);
//			memmove(newname,name,strlen(name));
			char buffer[200];
			char *k;
			k=strstr(in,"@set-name")+sizeof("@set-name");
		/*	if((strchr(k,'\n'))){
				*(strchr(k,'\n'))='\0';
					}*/
		//	k[strlen(k)-1]='\0';
			if(strlen(k)<20){
//				memmove(newname,k,strlen(k));
				sprintf(in,"@set-name %s:",k);
				msg=messageCrafter(name,"server",in);
				say(client_fd,msg);
				free(msg);
				recv(client_fd,buffer,200,0);
				if(strrchr(k,':'))
					*strrchr(k,':')='\0';

//				puts(buffer);
 				if(strstr(buffer,"updated")){
					memset(name,'\0',20);
					memmove(name,k,strlen(k));
					printf("\n\n\t\t\tname set to %s\t(serverReply)\n\n ",name);
				}
				else{
					printf("\n\n\t\t%s has already been taken\t(serverReply)\n\n",k);
					return -1;
				}
	/*			sprintf(in,"@set-name %s:",name);
 				msg=messageCrafter(oldname,"server",in);
				say(client_fd,msg);
				free(msg);*/
			}else{
				printf("%s is too big",k);
				return -1;
			}
		return 0;
		}
		msg=messageCrafter(name,"server",in);
	}
	say(client_fd,msg);
	free(msg);
	return 0;
}
