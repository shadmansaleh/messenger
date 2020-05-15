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

//#include "../../../messenger-attempt1/include/my_header.h"

#define MAX_CLIENTS 10

typedef struct{
	int index;
	int socket;
	char name[20];
//	struct sockaddr_in *clientaddr;
}client;


int server_fd;
client clients[MAX_CLIENTS];
//char names[MAX_CLIENTS][20];


//functions
void error(char *er);
int read_in(int fd,char *data,int dsize);
int say(int socket, char *m); 
char * messageCrafter(char *from,char *to,char *m);
void getto(char *usrto,char *msg);
void getfrom(char *usrfrom,char *msg);
void getmessage(char *message,char *msg);
void interpreteMessage(char *userfrom,char *userto,char *message,char *msg);
void listactiveusers(char *m);
int adduser(int,struct sockaddr_in*);
void msgDestributer(char *);
int removeuser(int id,int socket,char* name);
int getId(char *name);
void interpretemessage(char *userfrom,char *userto,char *message,char *msg);
void serverReply(char *f,char *m);
int gettoId(char*);
void handle_shutdown(int sig);
int _send(int fd,char *msg);
int _recv(int fd,char *msg,int size);

int main(int argc,char *argv[]){
	int port=0;
	if(argc!=2 || (port=atoi(argv[1])==0)){
		printf("%s [port]\n",argv[0]);
		exit(0);
	}
	port=atoi(argv[1]);
	int new_client,result,i,j,max_fd=0;
	struct sockaddr_in serveraddr,clientaddr;
	unsigned int addrlen=sizeof(clientaddr);
	char buffer[1025];
	for(i=0;i<MAX_CLIENTS;i++){
		clients[i].index=i;
		clients[i].socket=0;
		memset(clients[i].name,'\0',sizeof(clients[i].name));
		memmove(clients[i].name,"",sizeof(""));
	}

	fd_set fds;

	struct sigaction term;
	term.sa_handler=handle_shutdown;
	sigemptyset(&term.sa_mask);
	term.sa_flags=0;
	sigaction(SIGINT,&term,NULL);

	if((server_fd=socket(AF_INET,SOCK_STREAM,0))==0)
		error("Socket creation failed!");
	int reuse=1;
	if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int))==-1)
		error("Sockopt failed");
	serveraddr.sin_family=AF_INET;
	clientaddr.sin_family=AF_INET;
	serveraddr.sin_addr.s_addr=INADDR_ANY;
	serveraddr.sin_port=htons(port);
	if(bind(server_fd,(struct sockaddr *)&serveraddr,addrlen)==-1)
		error("Bind failed");
	if(listen(server_fd,5)==-1)
		error("Listen failed");
	printf("Waiting for clients at port %d\n",port);
	
	while(1){
		FD_ZERO(&fds);
		FD_SET(server_fd,&fds);
		max_fd=server_fd;
		for(i=0;i<MAX_CLIENTS;i++){
			if(clients[i].socket>0){
				FD_SET(clients[i].socket,&fds);
				if(clients[i].socket>max_fd)
					max_fd=clients[i].socket;
			}
				
		}
//		puts("starting select");
		j=select(max_fd+1,&fds,NULL,NULL,NULL);
		if(j<0){
			perror("select stoped");
			continue;
		}
		if(FD_ISSET(server_fd,&fds)){
			//new connection
			j--;
			if((new_client=accept(server_fd,(struct sockaddr *)&clientaddr,&addrlen))==-1)
				perror("Accept failed");
			else{
				result=adduser(new_client,&clientaddr);
				if(result==-1)
					perror("New client rejected");
			}if(j==0)
				continue;

		}
		else{
			//read clients
			for (i=0;i<MAX_CLIENTS;i++){
				if(clients[i].socket!=0){
					if(FD_ISSET(clients[i].socket,&fds)){
						j--;
						result=read_in(clients[i].socket,buffer,1025);
						if(result==0) {//check if user left
							printf("%s has left\n",clients[i].name);
							shutdown(clients[i].socket,SHUT_RDWR);
							close(clients[i].socket);
							clients[i].socket=0;
							memset(clients[i].name,'\0',20);
						//	removeuser(i,-1,NULL);
						}
						else
							msgDestributer(buffer);
					}
				}
				if(j==0)
					continue;
			}
		}

	}
	return 0;
}

void error(char *er){
	fprintf(stderr,"%s : %s\n",er,strerror(errno));
	exit(1);
}


/*int read_in(int fd,char *data,int dsize){ 
	int res=recv(fd,data,dsize,0);
	if(res==0)
		removeuser(-1,fd,NULL);
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

//	printf("received : %s\n",data);

       if(s==-1)  
	       return -1;
       else
   //            return i;   
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
//		removeuser(-1,fd,NULL);
		return 0;
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
//		removeuser(-1,fd,NULL);
		return 0;
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


/*int say(int socket, char *m) {
	int res=send(socket,m,strlen(m),0);
	return res;

//	char *s=m;
	int siz=strlen(s);
	int r=0;
	int result=0;
	int retry=0;
	int i=0;
	int id=-1;
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
			while(i<MAX_CLIENTS){
				if(clients[i].socket==socket){
					id=i;
					break;
				}
				i++;
			}
			removeuser(id,-1,NULL);
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

int say(int socket,char *m){
	int res=0;
	int len =strlen(m);
	char buff[100];
	sprintf(buff,"%d:",len);
send_size:
	res=_send(socket,buff);
	if(res==0)
//		removeuser(-1,socket,NULL);
		return 0;
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
//		removeuser(-1,socket,NULL);
		return 0;
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
//	char *msg=malloc(1045);
	sprintf(msg,"<to:%s><from:%s><msg:%s>!\n",to,from,m);
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

int gettoId(char *msg){
	if(strstr(msg,"<toId")==NULL)
		return -1;
	char *tmp=strstr(msg,"<toID:")+4;
	char uIDS[10];
	char *u=uIDS;
	while(*tmp!='>'){
 		*u=*tmp;
		u++;
		tmp++;
	}
	*u='\0';
//	printf("To is %s\n",usrto);
	return atoi(uIDS);
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

/*void listactiveusers(char *m){
	char *n=m;
	int i =0;
	char me[30]="Active Users are : \n";
	memmove(n,me,strlen(me));
	n+=(strlen(me)+1);
	while(i<MAX_CLIENTS){
		if(clients[i].socket!=0){
			me[0]='\t';
			me[1]='\0';
			strcat(me,clients[i].name);
			memmove(n,me,sizeof(char)*strlen(me));
			n=n+strlen(me);
			*n='\n';
			n++;
		}
		i++;
	}
}
*/
void listactiveusers(char *m){
	char buff[50];
	int i=0;
	int j=1;
	memset(buff,'\0',50);
	memmove(buff,"\n\t\tActive users are : \n\n",sizeof("\n\t\tActive users are : \n"));
	strcat(m,buff);
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i].socket!=0){
			memset(buff,'\0',50);
			sprintf(buff,"\t\t%d\t%s\n",j,clients[i].name);
			strcat(m,buff);
			j++;
		}
	}

}


int adduser(int socket,struct sockaddr_in *clientaddr){
	int id=-1;
	int i;
	char buffer[200];
	char *msg;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i].socket==0){
			id=i;
			break;
		}
	}
	if(id!=-1){
		//adding user
		clients[id].socket=socket;
		memset(clients[id].name,'\0',sizeof(clients[id].name));
		sprintf(clients[id].name,"USER%d",id);
		sprintf(buffer,"<id:%d><username:%s>",id,clients[id].name);
		msg=messageCrafter("server",clients[i].name,buffer);
		say(clients[i].socket,msg);
		free(msg);
		printf("New connection from IP:%s PORT:%d\n",inet_ntoa(clientaddr->sin_addr),ntohs(clientaddr->sin_port));	
	}
	else{
		sprintf(buffer,"Sorry Server full . Please try again later");
		msg=messageCrafter("server","unauthorized",buffer);
		say(socket,msg);
		free(msg);
	}
	return id;
}

int removeuser(int id,int socket,char *name){
	int i=-1;
	int j=0;
	if(id>=0)
		i=id;
	else if(socket>0){
		for(j=0;j<MAX_CLIENTS;j++){
			if(clients[j].socket==socket){
				i=j;
				break;
			}
		}
	}
	else if(name!='\0'){
		i=getId(name);
	}
//	printf("removing id:%d\n",i);
	if(i!=-1){
//		printf("%d -- %d\n",i,clients[i].socket);//=0;
	//	*(clients[i].name)='\0';
		shutdown(clients[i].socket,SHUT_RDWR);
		close(clients[i].socket);
		printf("%s has left\n",clients[i].name);
		memset(clients[i].name,'\0',sizeof(clients[i].name));
		clients[i].socket=0;
		return 1;
	}else 	return -1;
}

int getId(char *name){
	int i=0;
	int id=-1;
	while (i<MAX_CLIENTS){
		if(!strcmp(clients[i].name,name)){
			id=i;
			break;
		}
		i++;
	}//if(i==MAX_CLIENTS)	i= -1;
	return id;
}


void msgDestributer(char *msg){
	char from[20];
	char to[20];
	int toId=-1;
	char message[1000];
	interpretemessage(from,to,message,msg);
	if(!strcmp(to,"server"))
		serverReply(from,message);
	else if((toId=gettoId(msg))!=-1)
		say(clients[toId].socket,msg);
	else{
		if((toId=getId(to))!=-1)
			say(clients[toId].socket,msg);
		else{
			memset(message,'\0',1000);
			sprintf(message,"Message delivery failed\n%s is not connected to server . type @users to see whp are connected",to);
			msg=messageCrafter("server",from,message);
			say(clients[getId(from)].socket,msg);
			free(msg);
		}
	}
	
}


void serverReply(char *f,char *m){
	int Id=getId(f);
	char buffer[1000];
	char *msg=NULL;
//	char *n=NULL;
	if(strstr(m,"@help"))
		sprintf(buffer,"\n\n\tHELP\n\t\t@help      show this message\n\t\t@users     list active users\n\t\t@set-name  change your username\n\t\t@set-to    set the username of recipent\n\n");
	else if(strstr(m,"@users")){
		char us[25*MAX_CLIENTS];
		listactiveusers(us);
		msg=messageCrafter("server",clients[Id].name,us);
		say(clients[Id].socket,msg);
		free(msg);
		return;
	}
/*	else if((n=strstr(m,"@set-name"))!=NULL){
			char *k=clients[Id].name;
	//		memset(clients[Id].name,'\0',sizeof(clients[Id].name));
			for(int i=0;i<20;i++){
				clients[Id].name[i]='\0';
			}
			n+=sizeof("@set-name");
			while(*n!='\n'&& *n!=':'&&*n){
				*k=*n;
				k++;
				n++;
			}
			sprintf(buffer,"name has been updated to %s",clients[Id].name);
			}*/
	else if(strstr(m,"@set-name")){
			char *k;
			k=strstr(m,"@set-name")+sizeof("@set-name");
			//k[strlen(k)-1]='\0';
			*strrchr(k,':')='\0';
		/*	while((strrchr(k,':'))){
				*(strchr(k,':'))='\0';
					}*/
			if(getId(k)==-1){
				memset(clients[Id].name,'\0',strlen(clients[Id].name));
				memmove(clients[Id].name,k,strlen(k));
				sprintf(buffer,"name has been updated to %s",clients[Id].name);
			}
			else
				sprintf(buffer,"%s has already been taken",k);
			_send(clients[Id].socket,buffer);
			return;
		}

	else{
		sprintf(buffer,"%s Command not found. type @help to see available commands",m);
	}
	msg=messageCrafter("server",clients[Id].name,buffer);
//	send(clients[Id].socket,msg,sizeof(msg),0);
//change it
	say(clients[Id].socket,msg);
	free(msg);
}

void handle_shutdown(int sig) { 
	if (server_fd) { 
		shutdown(server_fd,SHUT_RDWR);
		close(server_fd);
	}
	int i=0;
	while (i<MAX_CLIENTS){
		if(clients[i].socket!=0){
			removeuser(-1,clients[i].socket,NULL);
		}
		i++;
	}
	fprintf(stderr, "\nBye!\n"); 
	exit(0);
}



