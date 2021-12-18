#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include <fcntl.h>


#define PORT 6666//定义端口号
#define IP "0.0.0.0"//查看自己的IP
#define ERR_LOG(send_msg) do{\
	fprintf(stderr, "%d %s ", __LINE__, __func__);\
	perror(send_msg);\
}while(0)


#define N 128


sqlite3 *db=NULL;

typedef struct message
{
	char type;//1注册 2登录 3查单词 4历史记录 5退出 登录(6错误 7正确)
	char name[30];
	char date[N];
	char time[50];
}msg;

int	do_sql();
int do_login(int newfd,msg log);
int do_regis(int newfd,msg log);
int do_input(int newfd,msg log1);
int do_inputp(int newfd,msg log,msg log1);
int do_input4(int newfd,msg log);
int do_quit(int newfd,msg log);

#define ERR_MSG(msg) do{fprintf(stderr,"__%d__:",__LINE__);perror(msg);}while(0)
void * callBackClInfo(void *arg);

struct cliInfo
{
	int newfd;
	struct sockaddr_in cin;//创建结构体用于传输线程资源
};


int main(int argc, const char *argv[])
{

	int sfd=socket(AF_INET,SOCK_STREAM,0);//创建流式套接字
	if(sfd<0){
		ERR_MSG("socket");
		return -1;
	}
	//允许端口快速重用
	int reuse =1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int))<0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}
	
	struct sockaddr_in sin;//绑定服务器的IP和端口，填充地址信息结构体
	sin.sin_family 	=AF_INET;
	sin.sin_port 	=htons(PORT);//端口号的网络字节序
	sin.sin_addr.s_addr = inet_addr(IP);//IP地址的网络字节序

	

	if(bind(sfd,(struct sockaddr *)&sin,sizeof(sin))<0)//绑定服务器的IP和端口
	{
		ERR_MSG("bind");
		return -1;
	}

	if(listen(sfd,10)<0)//将套接字设置成被监听
	{
		ERR_MSG("listen");
		return -1;
	}
	fprintf(stderr,"已成功监听\n");
	struct sockaddr_in sin1;
	socklen_t a=sizeof(sin);
	pthread_t tid;

	if(sqlite3_open("./sq.db",&db)!=SQLITE_OK)//打开词库
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}

	char sql[128]="create table if not exists ppr(name char,word char,time char)";
	char *errmsg =NULL;
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;  
	}
	char sql1[128]="create table if not exists pow(name char primary key,mima char,type char)";
	if(sqlite3_exec(db,sql1,NULL,NULL,&errmsg)!=SQLITE_OK)//创建密码数据库
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;  
	}
	char sqp[108]= "create table if not exists stu(danci char,zhushi char);";
	if(sqlite3_exec(db,sqp,NULL,NULL,&errmsg)!=0)                               
	{
		printf("%s\n",errmsg);
		return -1;
	}

	do_sql();


	while(1)
	{
		int newfd=accept(sfd,(struct sockaddr *)&sin1,&a);//获取已经完成链接的套接字
		if(newfd<0)
		{
			ERR_MSG("accept");
			return -1;
		}
		printf("[%s:%d] newfd=%d 链接成功\n" , inet_ntoa(sin1.sin_addr), ntohs(sin1.sin_port), newfd);//主线程用于处理链接问题
		
		struct cliInfo cli;
		cli.newfd=newfd;
		cli.cin=sin;
		if(pthread_create(&tid,NULL,callBackClInfo,(void *)&cli)!=0)//创建线程用于交互
		{
			ERR_MSG("pthred_create");
			return -1;
		}

	}
	close(sfd);
	return 0;
}

int do_sql()
{
	char sql[200]="select * from stu;";
	char sqp[200]="drop table stu;";
	char ** result=NULL;
	int row,column;
	char * errmsg=NULL;
	if(sqlite3_get_table(db,sql,&result,&row,&column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));   
		return -1;
	}
	if(row==7987)
	{
		printf("词库无误，可以使用\n");
	}
	else
	{
		printf("词库被污染，重新下载\n");
		if(sqlite3_get_table(db,sqp,&result,&row,&column,&errmsg)!=SQLITE_OK)
		{
			printf("%s\n",sqlite3_errmsg(db));   
			return -1;
		}
		printf("污染库已被删除\n");
		printf("词库正在重新下载\n");
		char sq[108]= "create table if not exists stu(danci char,zhushi char);";
		if(sqlite3_exec(db,sq,NULL,NULL,&errmsg)!=0)                               
		{
			printf("%s\n",errmsg);
			return -1;
		}

		int fd=open("./dict.txt",O_RDONLY);
		if(fd<0)
		{
			perror("open");
			return -1;
		}
		char c;                                                              
		char brr[30];
		char crr[100];
		bzero(brr,sizeof(brr));
		bzero(crr,sizeof(brr));
		int i=0,j=0,flag=0;
		ssize_t size;
		while(1)
		{
			size=read(fd,&c,1);
			if(size==0)
			{
				printf("词库录取完成,请使用\n");
				break;
			}
			if(c!=' '&&flag==0)
			{
				brr[i++]=c;
				continue;
			}
			i=0;
			flag=1;
			if(c!='\n'&&flag==1)
			{
				crr[j++]=c;
				continue;
			}
			crr[j]='\0';
			j=0;
			flag=0;

			char sql1[128] = "";
			sprintf(sql1, "insert into stu values(\"%s\", \"%s\")",brr,crr);
			if(sqlite3_exec(db,sql1,NULL,NULL,&errmsg)!=0)
			{
				printf("%s\n",errmsg);
				return -1;
			}
			bzero(brr,sizeof(brr));
			bzero(crr,sizeof(brr));
		}
		close(fd);
	}
	return 0;
}


void * callBackClInfo(void *arg)
{
	pthread_detach(pthread_self());//分离线程，让线程自己回收资源,pthread_self（）获得当前线程的线程号
	struct cliInfo cli=*(struct cliInfo *)arg;
	int newfd = cli.newfd;
	struct sockaddr_in sin= cli.cin;
	ssize_t res =0;
	msg log;
	char buf[sizeof(log)];
PP:
PL:
	while(1)
	{
		res=recv(newfd,buf,sizeof(buf),0);//接受客户端信息
		if(res<0){
			ERR_MSG("recv");
			break;
		}
		else if(0==res)//判断客户端是否退出
		{
			fprintf(stderr,"客户端关闭\n");
			break;
		}
		log= *(msg *)buf;
		if(log.type=='1')
		{
			do_regis(newfd,log);
			goto PP;
		}
		else if(log.type=='2')
		{
			do_login(newfd,log);
			goto PL;
		}
		else if(log.type=='5')
		{
		}
	}
	close(newfd);//关闭不使用的文件描述符，防止文件描述符不够用，一共只有1021个能使用
	pthread_exit(NULL);//关闭线程
}

int do_quit(int newfd,msg log)
{
	char ** result=NULL;
	int row,column;
	char * errmsg=NULL;
	char sqp[200]="";
	sprintf(sqp,"update pow set type='Y' where name='%s';",log.name);
	printf("%s\n",sqp);
	if(sqlite3_get_table(db,sqp,&result,&row,&column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));   
		return -1;
	}	
	return 0;
}

int do_regis(int newfd,msg log)
{

	char sql[200]="select name from pow;";
	char ** result=NULL;
	int row,column;
	char * errmsg=NULL;
	char b='Y';
	if(sqlite3_get_table(db,sql,&result,&row,&column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));   
		return -1;
	}
	if(row!=0)
	{
		for(int i=0;i<row;i++)
		{
			if(strcmp(result[i+1],log.name)==0)
			{
				log.type='6';
				goto PPQ;
			}
			log.type='7';
		}
	}
	else
	{
		log.type='7';
	}
	if(log.type=='7')
	{
		bzero(sql,sizeof(sql));
		sprintf(sql,"insert into pow values(\"%s\",\"%s\",\"%c\");",log.name,log.date,b);
		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) !=0) 
		{
			printf("sqlite3_exec:%s %d\n", errmsg, __LINE__); 
			return -1;
		}
		printf("*****插入成功*****\n");
	}
PPQ:
	if(send(newfd,(void *)&log,sizeof(log),0)<0)//发送信息给客户端
	{
		ERR_MSG("send");
		return -1;
	}
	printf("发送数据成功\n");
	

	return 0;
}

int do_login(int newfd,msg log)
{
	char sql[200]="select *from pow;";
	char sqp[200]="";
	char ** result=NULL;
	int row,column;
	char * errmsg=NULL;
	if(sqlite3_get_table(db,sql,&result,&row,&column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));   
		return -1;
	}	
	if(row!=0)
	{
		for(int i=0;i<row*column;i++)
		{
			if(i%3==0){
				if(strcmp(result[3+i],log.name)==0)
				{
					if(strcmp(result[i+4],log.date)==0)
					{
						if(strcmp(result[i+5],"Y")==0)
						{
							log.type='7';
							break;
						}
					}
				}
			}
			log.type='6';
		}
	}
	else
	{
		log.type='6';
	}
	if(log.type=='7')
	{
		sprintf(sqp,"update pow set type='N' where name='%s';",log.name);
		printf("%s\n",sqp);
		if(sqlite3_get_table(db,sqp,&result,&row,&column,&errmsg)!=SQLITE_OK)
		{
			printf("%s\n",sqlite3_errmsg(db));   
			return -1;
		}	
	}
	if(send(newfd,(void *)&log,sizeof(log),0)<0)//发送信息给客户端
	{
		ERR_MSG("send");
		return -1;
	}
	if(log.type=='7')
	{	
		do_input(newfd,log);
	}
	return 0;
}

int do_input(int newfd,msg log1)
{
	msg log;
	ssize_t res =0;
	
	char buf[sizeof(log)];
PPD:
PPX:
	res=recv(newfd,buf,sizeof(buf),0);//接受客户端信息
	if(res<0){
		ERR_MSG("recv");
		return -1;
	}
	else if(0==res)//判断客户端是否退出
	{
		fprintf(stderr,"客户端关闭\n");
		return -1;
	}
	log= *(msg *)buf;
	if(log.type=='3')//查单词
	{
		do_inputp(newfd,log,log1);
		goto PPD;
	}
	else if(log.type=='4')//查历史
	{
		do_input4(newfd,log);
		goto PPX;
	}
	else if(log.type=='5')
	{
		do_quit(newfd,log1);
		printf("退出函数已结束\n");
	}
	return 0;
}

int do_input4(int newfd,msg log)
{

	char sql[128]="select *from ppr";                                           
	char ** result=NULL;
	int row,column;
	char * errmsg=NULL;
	if(sqlite3_get_table(db,sql,&result,&row,&column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}
	int i=0;
	int line,list;
	log.type='4';
	for(line=0;line<row+1;line++)
	{
		for(list=0;list<column;list++)
		{
			strcpy(log.name,result[i++]);
			if(send(newfd,(void *)&log,sizeof(log),0)<0)//发送信息给客户端
			{
				ERR_MSG("send");
				return -1;
			}
		}
	}
	log.type='5';
	if(send(newfd,(void *)&log,sizeof(log),0)<0)//发送信息给客户端
	{
		ERR_MSG("send");
		return -1;
	}
	sqlite3_free_table(result);

	return 0;
}

int do_inputp(int newfd,msg log,msg log1)
{
	char sql[200]="";
	char sqp[200]="";
	char sqq[60]="";
	sprintf(sql,"select zhushi from stu where danci=\"%s\";",log.name);
	char ** result=NULL;
	int row,column;
	char * errmsg=NULL;
	printf("%s\n",sql);
	if(sqlite3_get_table(db,sql,&result,&row,&column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));   
		return -1;
	}
	if(row==0)
	{ 	
		log.type='6';
		goto OO;
	}
	else if(row>0)
	{
		for(int i=0;i<row;i++)
		{
			strcpy(log.date,result[1+i]);
	
			log.type='7';	
			if(send(newfd,(void *)&log,sizeof(log),0)<0)//发送信息给客户端
			{
				ERR_MSG("send");
				return -1;
			}
			time_t timep;
			struct tm *p;
			time(&timep);
			p=localtime(&timep);
			sprintf(sqq,"%d-%d-%d-%d-%d",1900+p->tm_year,1+p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min);
			sprintf(sqp,"insert into ppr values(\"%s\",\"%s\",\"%s\");",log1.name,log.name,sqq);

			if(sqlite3_exec(db,sqp,NULL,NULL,&errmsg)!=SQLITE_OK)          
			{
				printf("%s\n",sqlite3_errmsg(db));
				return -1;      
			}
		}
		log.type='5';
OO:
		if(send(newfd,(void *)&log,sizeof(log),0)<0)//发送信息给客户端
		{
			ERR_MSG("send");
			return -1;
		}
	}

	return 0;
}
