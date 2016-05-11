#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<linux/in.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<pthread.h>
#include<malloc.h>
#define BUFFLEN 1024
#define SERVER_PORT 8888
#define MTLEN 128

typedef struct msgmbuf {
	long rtype;
	long stype;
	char mtext[MTLEN];
	}MSG;
typedef struct uaccount{
	//int anum;
	char uname[32];
	char upwd[32];
	}UAT;//user account
typedef struct dbnode{
	long unum;//user number
	char uname[32];
	char ustat;
	struct dbnode * next;
	}DBN;//database node


char s_buff[BUFFLEN];
char r_buff[BUFFLEN];
char cmd[MTLEN];
MSG sendbuff,recvbuff;
MSG *sp,*rp;
UAT myaccount,*ap;
DBN head;


int main(int argc,char * argv[])
{
	int sc;
	struct sockaddr_in server;
	sp=&sendbuff;
	rp=&recvbuff;
	ap=&myaccount;
	char buff[BUFFLEN];
	ssize_t m=0,n=0;
	pthread_t stid,rtid;
	void * rmsg(void *s_c);
	void * smsg(void *s_c);
	sc=socket(AF_INET,SOCK_STREAM,0);
	memset(&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(SERVER_PORT);	
	int err=connect(sc,(struct sockaddr *)&server,sizeof(server));
	if(err<0)
	{
		printf("Can't not connect server!\n");
		printf("error:%s\n",strerror(errno));
		return -1;
	}
	else;
	printf("\n\t\tWelcome to ChatHappy!\n\n");
	while(1)
	{
		printf("Log in(L) or Sign up(S)?(input L or S):");
		gets(cmd);
		if(cmd[0]=='L')
		{
			printf("Login begins...\n");
			printf("username: ");
			gets(ap->uname);
			printf("password: ");
			gets(ap->upwd);
			memset(s_buff,0,BUFFLEN);
			s_buff[0]=cmd[0];
			strcpy(s_buff+1,ap->uname);
			strcpy(s_buff+33,ap->upwd);
			write(sc,s_buff,sizeof(UAT));
			memset(r_buff,0,BUFFLEN);
			n=recv(sc,r_buff,BUFFLEN,0);
			if(n<0)
			{
				printf("recv error!\n");
				return -2;
			}
			else;
			if(r_buff[1]=='0')
			{
				printf("\nlogin successfully!\n");
				printf("\ntips:Add_friend(A),Chat(C),Del_friend(D),Get_friendlist(G),Quit(Q)\n");
				printf("input fomat is like:C,jyj,hello world!\n");
				sp->stype=(long)r_buff[2];
				break;
			}
			else if(r_buff[1]=='1')
			{
				printf("user doesn't exist!\n");
				continue;
			}
			else if(r_buff[1]=='2')
			{
				printf("wrong password!\n");
				continue;
			}
			else
			{
				printf("undefined state!\n");
				continue;
			}
		}
		else if(cmd[0]=='S')
		{
			printf("Sign up begins...\n");
			printf("your username: ");
			gets(ap->uname);
			printf("your password: ");
			gets(ap->upwd);
			memset(s_buff,0,BUFFLEN);
			s_buff[0]=cmd[0];
			strncpy(s_buff+1,ap->uname,strlen(ap->uname));
			strncpy(s_buff+33,ap->upwd,strlen(ap->upwd));
			write(sc,s_buff,sizeof(UAT));
			memset(r_buff,0,BUFFLEN);
			n=recv(sc,r_buff,BUFFLEN,0);
			if(n<0)
			{
				printf("recv error!\n");
				return -3;
			}
			else;
			if(r_buff[1]=='0')
			{
				printf("\nsign up successfully!\n");
				printf("\ntips:Add_friend(A),Chat(C),Del_friend(D),Get_friendlist(G),Quit(Q)\n");
				printf("input fomat is like:C,jyj,hello world!\n");
				sp->stype=(long)r_buff[2];
				break;
			}
			else if(r_buff[1]=='1')
			{
				printf("user already exist!\n");
				continue;
			}
			else
			{
				printf("undefined state!\n");
				continue;
			}
		}
		else
		{
			printf("invalid input!\n");
			continue;
		}
	}
	
	pthread_create(&stid,NULL,smsg,(void *)&sc);
	pthread_create(&rtid,NULL,rmsg,(void *)&sc);
	pthread_join(stid,NULL);
	pthread_join(rtid,NULL);
	close(sc);
	return 0;
}


void * rmsg(void *s_c)
{
	int sc=*((int *)s_c);
	ssize_t n=0;
	int i,j;
	DBN * dp;
	char uname_tmp[32];
	void *pm=NULL;
	DBN *tmp;
	while(1)
	{
		memset(r_buff,0,BUFFLEN);
		n=recv(sc,r_buff,BUFFLEN,0);//MSG_DONTWAIT
		if(n>0)
		{
			switch(r_buff[0])
			{
				case 'A':
					if(r_buff[1]=='E')
					{
						printf("no such user on server database!\n");
						break;
					}
					else if(r_buff[1]=='N')
					{
						printf("Sorry,%s don't want to be your friend!\n",r_buff+2);
						break;
					}
					else if(r_buff[1]=='R')
					{
						printf("Hey,%s want to be your friend!\n",r_buff+2);
						break;
					}
					else if(r_buff[1]=='Y')
					{
						printf("Congratulations!%s is your friend now!\n",r_buff+2);
						break;
					}
					else
					{
						printf("invalid add_friend state!\n");
					}
					break;

				case 'D':
					printf("Sadly,%s is not your friend now!\n",r_buff+1);
					break;
				
				case 'C':
					if(r_buff[1]=='E')
					{
						if(r_buff[2]=='1')
						{
							printf("Can not chat with the user who have not signed up to the server!\n");
							break;
						}
						else if(r_buff[2]=='2')
						{
							printf("the one you want to chat with is not your friend now!\n");
							break;
						}
						else;
					}
					else;
					memset(uname_tmp,0,32);
					strncpy(uname_tmp,r_buff+1,32);
					printf("receive from %s:",uname_tmp);
					printf("%s\n",r_buff+33);
					break;

				case 'G':
					printf("\nyour friend list:\n");
					printf("username\tonline state\n");
					for(i=0;r_buff[i*33+1]!='\0';i++)
					{
						memset(uname_tmp,0,32);
						strncpy(uname_tmp,r_buff+i*33+1,32);
						printf("%s\t\t%c\n",uname_tmp,r_buff[i*33+33]);
					}
					break;

				default:
					perror("rmsg");
					return -7;
			}
		}
	}
}

void *smsg(void *s_c)
{
	int sc=*((int *)s_c),err;
	ssize_t n=0;
	int i,j;
	DBN *dp;
	char uname_tmp[32];
	void *pm=NULL;
	while(1)
	{
		memset(cmd,0,MTLEN);
		gets(cmd);
		s_buff[0]=cmd[0];
		switch(cmd[0])
		{
			case 'A':
				memset(s_buff,0,BUFFLEN);
				s_buff[0]=cmd[0];
				if(cmd[2]=='R')
				{
					s_buff[1]='R';
					printf("The request for adding friend have been sent!\n");
				}
				else if(cmd[2]=='Y')
				{
					s_buff[1]='Y';
				}
				else if(cmd[2]=='N')
				{
					s_buff[1]='N';
				}
				else
				{
					printf("invalid input!\n");
					break;
				}
				strcpy(s_buff+2,cmd+4);
				err=write(sc,s_buff,sizeof(s_buff));
				if(err<0)
				{
					printf("error:%s\n",strerror(errno));
					return -4;
				}
				break;
			
			case 'D':
				printf("Del_friend successfully!\n");
				memset(s_buff,0,BUFFLEN);
				s_buff[0]=cmd[0];
				strcpy(s_buff+1,cmd+2);
				write(sc,s_buff,sizeof(s_buff));
				break;

			case 'Q':
				memset(s_buff,0,BUFFLEN);
				s_buff[0]='Q';
				write(sc,s_buff,strlen(s_buff));
				printf("Quit the chattool!\n");
				exit(0);
			
			case 'C':
				memset(uname_tmp,0,32);
				for(i=2,j=0;cmd[i]!=':';i++,j++)
				{
					uname_tmp[j]=cmd[i];
				}
				memset(s_buff,0,BUFFLEN);
				s_buff[0]='C';
				strncpy(s_buff+1,uname_tmp,32);
				strcpy(s_buff+33,cmd+i+1);
				write(sc,s_buff,sizeof(s_buff));
				break;
			
			case 'G':
				memset(s_buff,0,BUFFLEN);
				s_buff[0]='G';
				write(sc,s_buff,strlen(s_buff));
				break;

			case 'M':
				printf("\ntips:Add_friend(A),Chat(C),Del_friend(D),Get_friendlist(G),Quit(Q)\n");
				break;

			default:
				printf("invalid input!\n");
		}
				
	}
}
