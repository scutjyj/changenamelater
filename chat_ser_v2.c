#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<linux/in.h>
#include<fcntl.h>
#include<errno.h>
#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 2
#define MTLEN 128
typedef struct msgmbuf{
	long rtype;
	long stype;
	char mtext[MTLEN];
	}MSG;
typedef struct flnode{
	long unum;
	struct flnode *next;
	}FLN;//friend list node
typedef struct dbnode{
	long unum;
	char uname[32];
	char upwd[32];
	char ustat;
	FLN head;
	struct dbnode *next;
	}DBN;

pthread_mutex_t mutex;
long i=1;
DBN head;

int main(int argc,char * argv[])
{
	int ss;
	struct sockaddr_in local;
	int err;
	void handle_conect(int ss);
	ss=socket(AF_INET,SOCK_STREAM,0);
	pthread_mutex_init(&mutex,NULL);
	memset(&local,0,sizeof(local));
	local.sin_family=AF_INET;
	local.sin_addr.s_addr=htonl(INADDR_ANY);
	local.sin_port=htons(SERVER_PORT);
	
	err=bind(ss,(struct sockaddr *)&local,sizeof(local));
	err=listen(ss,BACKLOG);
	
	handle_connect(ss);
	pthread_mutex_destroy(&mutex);
	close(ss);
	return 0;
}


int handle_connect(int ss)
{
	int sc;
	struct sockaddr_in client;
	void * session_handle(void * s_c);
	pthread_t tid;
	int len=sizeof(client);
	while(1)
	{
		sc=accept(ss,(struct sockaddr *)&client,&len);
		if(sc>0)
		{	
			pthread_create(&tid,NULL,session_handle,(void *)&sc);
		}
		else
		{
			perror("accept");
		}
	}
}

void * session_handle(void * s_c)
{
	pthread_detach(pthread_self());
	int sc=*((int *)s_c);
	char s_buff[BUFFLEN];
	char r_buff[BUFFLEN];
	char uname_tmp[32];
	char upwd_tmp[32];
	ssize_t n=0;
	MSG session_buff,*p;
	p=&session_buff;
	key_t key;
	int msgid,ret;
	int m;//circle variable to count the friend list user amount
	long myunum,ounum;//other unum
	DBN *dp,*tdp;//temporate dp
	FLN *fp,*tfp;//temporate fp
	while(1)
	{
		memset(r_buff,0,BUFFLEN);
		n=recv(sc,r_buff,BUFFLEN,0);
		if(n>0)
		{
			switch(r_buff[0])
			{
				case 'S':
					dp=&head;
					memset(uname_tmp,0,32);
					memset(upwd_tmp,0,32);
					strncpy(uname_tmp,r_buff+1,32);
					pthread_mutex_lock(&mutex);
					strncpy(upwd_tmp,r_buff+33,32);
					while(1)
					{
						if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
						{
							pthread_mutex_unlock(&mutex);
							memset(s_buff,0,BUFFLEN);
							s_buff[0]='S';
							s_buff[1]='1';
							write(sc,s_buff,strlen(s_buff));
							printf("%s\n",s_buff);
							break;
						}
						else if(dp->next==NULL)
						{
							if(i!=1)
							{
								dp->next=(DBN *)malloc(sizeof(DBN));
								dp=dp->next;
							}
							dp->unum=i;
							(dp->head).unum=i;
							strncpy(dp->uname,uname_tmp,strlen(uname_tmp));
							strncpy(dp->upwd,upwd_tmp,32);
							dp->ustat='Y';
							s_buff[0]='S';
							s_buff[1]='0';
							s_buff[2]=(char)i;
							myunum=i;
							i++;
							pthread_mutex_unlock(&mutex);
							write(sc,s_buff,strlen(s_buff));
							printf("%s\n",s_buff);
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					dp=&head;
					printf("unum\tuname\tupwd\tustat\n");
					while(dp->unum!=0)
					{
						printf("%d\t%s\t%s\t%c\n",dp->unum,dp->uname,dp->upwd,dp->ustat);
						if(dp->next==NULL)
						{
							break;
						}
						dp=dp->next;
					}
					break;

				case 'L':
					dp=&head;
					memset(uname_tmp,0,32);
					memset(upwd_tmp,0,32);
					strncpy(uname_tmp,r_buff+1,32);
					pthread_mutex_lock(&mutex);
					strncpy(upwd_tmp,r_buff+33,32);
					while(1)
					{
						if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
						{
							if(!strncmp(dp->upwd,upwd_tmp,strlen(upwd_tmp)))
							{
								s_buff[0]='S';
								s_buff[1]='0';
								s_buff[2]=(char)dp->unum;
								dp->ustat='Y';
								myunum=dp->unum;
								pthread_mutex_unlock(&mutex);
								write(sc,s_buff,strlen(s_buff));
								break;
							}
							else
							{
								pthread_mutex_unlock(&mutex);
								s_buff[0]='S';
								s_buff[1]='2';
								write(sc,s_buff,strlen(s_buff));
								break;
							}
						}
						else if(dp->next==NULL)
						{
							pthread_mutex_unlock(&mutex);
							s_buff[0]='S';
							s_buff[1]='1';
							write(sc,s_buff,strlen(s_buff));
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					break;	
				}
			}
			break;
	}		
	MSG msg;
	key=ftok("./",108);
	if(key==-1)
	{
		perror("ftok");
		return -1;
	}
	msgid=msgget(key,IPC_CREAT|0666);
	if(msgid<0)
	{
		perror("msgget");
		return -2;
	}
	while(1)
	{
		memset(r_buff,0,BUFFLEN);
		n=recv(sc,r_buff,BUFFLEN,MSG_DONTWAIT);
		if(n>0)
		{
			write(1,r_buff,BUFFLEN);
			switch(r_buff[0])
			{
				case 'A':
					if(r_buff[1]=='R')
					{
						dp=&head;
						memset(uname_tmp,0,32);
						strncpy(uname_tmp,r_buff+2,32);
						pthread_mutex_lock(&mutex);
						while(1)
						{
							if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
							{
								msg.rtype=dp->unum;
								pthread_mutex_unlock(&mutex);
								msg.stype=myunum;
								msg.mtext[0]='A';
								printf("%d,%d,%s\n",msg.rtype,msg.stype,msg.mtext);
								ret=msgsnd(msgid,&msg,sizeof(MSG)-sizeof(long),0);
								if(ret<0)
								{perror("msgsnd");}
								break;
							}
							else if(dp->next==NULL)
							{
								pthread_mutex_unlock(&mutex);
								s_buff[0]='A';
								s_buff[1]='E';
								write(sc,s_buff,strlen(s_buff));
								break;
							}
							else
							{
								dp=dp->next;
							}
						}
					}
					else if(r_buff[1]=='N')
					{
						dp=&head;
						memset(uname_tmp,0,32);
						strncpy(uname_tmp,r_buff+2,32);
						pthread_mutex_lock(&mutex);
						while(1)
						{
							if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
							{
								msg.rtype=dp->unum;
								pthread_mutex_unlock(&mutex);
								msg.stype=myunum;
								msg.mtext[0]='N';
								ret=msgsnd(msgid,&msg,sizeof(MSG)-sizeof(long),0);
								if(ret<0)
								{perror("msgsnd");}
								break;
							}
							else
							{
								dp=dp->next;
							}
						}
					}
					else if(r_buff[1]=='Y')
					{
						dp=&head;
						memset(uname_tmp,0,32);
						strncpy(uname_tmp,r_buff+2,32);
						pthread_mutex_lock(&mutex);
						while(1)
						{
							if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
							{
								msg.rtype=dp->unum;
								ounum=dp->unum;
								msg.stype=myunum;
								msg.mtext[0]='Y';
								ret=msgsnd(msgid,&msg,sizeof(MSG)-sizeof(long),0);
								if(ret<0)
								{perror("msgsnd");}
								fp=&(dp->head);
								while(1)
								{
									if(fp->unum==myunum)
									{break;}
									else if(fp->next==NULL)
									{			
										fp->next=(FLN *)malloc(sizeof(FLN));
										fp=fp->next;
										fp->unum=myunum;
										break;
									}
									else
									{
										fp=fp->next;
									}
								}
								break;
							}
							else
							{
								dp=dp->next;
							}
						}
						dp=&head;
						while(1)
						{
							if(dp->unum==myunum)
							{
								fp=&(dp->head);
								while(1)
								{
									if(fp->unum==ounum)
									{
										pthread_mutex_unlock(&mutex);
										break;
									}
									else if(fp->next==NULL)
									{			
										fp->next=(FLN *)malloc(sizeof(FLN));
										fp=fp->next;
										fp->unum=ounum;
										pthread_mutex_unlock(&mutex);
										break;
									}
									else
									{
										fp=fp->next;
									}
								}
								break;
							}
							else
							{
								dp=dp->next;
							}
						}
					}
					break;

				case 'D':
					dp=&head;
					memset(uname_tmp,0,32);
					strncpy(uname_tmp,r_buff+1,32);
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
						{
							msg.rtype=dp->unum;
							ounum=dp->unum;
							msg.stype=myunum;
							msg.mtext[0]='D';
							ret=msgsnd(msgid,&msg,sizeof(MSG)-sizeof(long),0);
							if(ret<0)
							{perror("msgsnd");}
							tfp=fp=&(dp->head);
							while(1)
							{
								if(fp->unum==myunum)
								{
									tfp->next=fp->next;
									free((void *)fp);
									break;
								}
								else if(fp->next==NULL)
								{			
									break;
								}
								else
								{
									tfp=fp;
									fp=fp->next;
								}
							}
							break;
						}
						else if(dp->next==NULL)
						{
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					dp=&head;
					while(1)
					{
						if(dp->unum==myunum)
						{
							tfp=fp=&(dp->head);
							while(1)
							{
								if(fp->unum==ounum)
								{
									tfp->next=fp->next;
									free((void *)fp);
									pthread_mutex_unlock(&mutex);
									break;
								}
								else if(fp->next==NULL)
								{			
									pthread_mutex_unlock(&mutex);
									break;
								}
								else
								{
									tfp=fp;
									fp=fp->next;
								}
							}
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					break;

				case 'G':
					dp=&head;
					memset(s_buff,0,BUFFLEN);
					s_buff[0]='G';
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==myunum)
						{
							fp=&(dp->head);
							m=0;
							while(1)
							{
								if(fp->next!=NULL)
								{
									tdp=&head;
									while(1)
									{
										if(tdp->unum==fp->unum)
										{
											strncpy(s_buff+1+m*33,tdp->uname,32);
											s_buff[33+m*33]=tdp->ustat;
											m++;
											break;
										}
										else
										{
											tdp=tdp->next;
										}
									}
									fp=fp->next;
								}
								else
								{
									tdp=&head;
									while(1)
									{
										if(tdp->unum==fp->unum)
										{
											strncpy(s_buff+1+m*33,tdp->uname,32);
											s_buff[33+m*33]=tdp->ustat;
											break;
										}
										else
										{
											tdp=tdp->next;
										}
									}
									break;
								}
							}
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					pthread_mutex_unlock(&mutex);
					write(sc,s_buff,sizeof(s_buff));
					break;

				case 'Q':
					dp=&head;
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==myunum)
						{
							dp->ustat='N';
							pthread_mutex_unlock(&mutex);
							break;
						}
						else if(dp->next==NULL)
						{
							pthread_mutex_unlock(&mutex);
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					break;

				case 'C':
					dp=&head;
					ounum=0;
					memset(uname_tmp,0,32);
					strncpy(uname_tmp,r_buff+1,32);
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(!strncmp(dp->uname,uname_tmp,strlen(uname_tmp)))
						{
							ounum=dp->unum;
							break;
						}
						else if(dp->next==NULL)//prevent send message to the user who doesn't sign up to the server
						{
							pthread_mutex_unlock(&mutex);
							s_buff[0]='C';
							s_buff[1]='E';
							s_buff[2]='1';//1st kind of chat error
							write(sc,s_buff,strlen(s_buff));
							break;
						}
						else
						{
							dp=dp->next;
						}
					}
					if(ounum!=0)
					{
						dp=&head;
						while(1)
						{
							if(dp->unum==myunum)
							{
								fp=&(dp->head);
								while(1)
								{
									if(fp->unum==ounum)
									{
										msg.rtype=ounum;
										pthread_mutex_unlock(&mutex);
										msg.stype=myunum;
										msg.mtext[0]='C';
										strncpy(msg.mtext+1,r_buff+33,MTLEN-1);
										ret=msgsnd(msgid,&msg,sizeof(MSG)-sizeof(long),0);
										if(ret<0)
										{perror("msgsnd");}
										break;
									}
									else if(fp->next==NULL)
									{
										pthread_mutex_unlock(&mutex);
										s_buff[0]='C';
										s_buff[1]='E';
										s_buff[2]='2';//2nd kind of chat error
										write(sc,s_buff,strlen(s_buff));
										break;
									}
									else
									{
										fp=fp->next;
									}
								}
								break;
							}
							else
							{
								dp=dp->next;
							}
						}
					}
					break;

				default:
					printf("invaile state!\n");
				}
					
			}
									
		
		ret=msgrcv(msgid,&msg,sizeof(MSG)-sizeof(long),myunum,IPC_NOWAIT);
		if(ret<0);
		else
		{
			switch(msg.mtext[0])
			{
				case 'A':
					memset(s_buff,0,BUFFLEN);
					s_buff[0]='A';
					s_buff[1]='R';
					dp=&head;
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==msg.stype)
						{
							strncpy(s_buff+2,dp->uname,strlen(dp->uname));
							pthread_mutex_unlock(&mutex);
							write(sc,s_buff,strlen(s_buff));
							break;
						}
						else 
						{
							dp=dp->next;
						}
					}
					break;

				case 'N':
					memset(s_buff,0,BUFFLEN);
					s_buff[0]='A';
					s_buff[1]='N';
					dp=&head;
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==msg.stype)
						{
							strncpy(s_buff+2,dp->uname,strlen(dp->uname));
							pthread_mutex_unlock(&mutex);
							write(sc,s_buff,strlen(s_buff));
							break;
						}
						else 
						{
							dp=dp->next;
						}
					}
					break;

				case 'Y':
					memset(s_buff,0,BUFFLEN);
					s_buff[0]='A';
					s_buff[1]='Y';
					dp=&head;
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==msg.stype)
						{
							strncpy(s_buff+2,dp->uname,strlen(dp->uname));
							pthread_mutex_unlock(&mutex);
							write(sc,s_buff,strlen(s_buff));
							break;
						}
						else 
						{
							dp=dp->next;
						}
					}
					break;

				case 'D':
					memset(s_buff,0,BUFFLEN);
					s_buff[0]='D';
					dp=&head;
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==msg.stype)
						{
							strncpy(s_buff+1,dp->uname,strlen(dp->uname));
							pthread_mutex_unlock(&mutex);
							write(sc,s_buff,strlen(s_buff));
							break;
						}
						else 
						{
							dp=dp->next;
						}
					}
					break;

				case 'C':
					memset(s_buff,0,BUFFLEN);
					s_buff[0]='C';
					dp=&head;
					pthread_mutex_lock(&mutex);
					while(1)
					{
						if(dp->unum==msg.stype)
						{
							strncpy(s_buff+1,dp->uname,32);
							pthread_mutex_unlock(&mutex);
							strcpy(s_buff+33,msg.mtext+1);
							write(sc,s_buff,sizeof(s_buff));
							break;
						}
						else 
						{
							dp=dp->next;
						}
					}
					break;
					
				default:
					printf("invalid state!\n");
			}

		}
	}
}

