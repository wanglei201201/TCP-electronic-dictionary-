#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>        
#include <unistd.h>
#include <signal.h>

#define ERR_MSG(msg)    do{\
    fprintf(stderr, "__%d__ :", __LINE__);\
    perror(msg); \
}while(0)

#define PORT 6666
#define IP "0.0.0.0"
#define ERR_LOG(send_msg) do{\
	fprintf(stderr, "%d %s ", __LINE__, __func__);\
	perror(send_msg);\
}while(0)

#define N 128
int do_login(int sfd);
int do_regis(int sfd);
int do_input(int sfd);
int do_quit(int sfd);
typedef struct message
{
	char type;//1注册 2登录 3查单词 4历史记录 5退出 登录or注册(6错误 7正确)
	char name[30];
	char date[N];
	char time[50];
}msg;

int sfd;

typedef void (*sighandler_t)(int);

void handler(int sig)
{
	do_quit(sfd);
	//kill(getpid(),9);
	//
	exit(0);
	return ;
}

int main(int argc, const char *argv[])
{
	sfd=socket(AF_INET,SOCK_STREAM,0);
	if(sfd<0)
	{
		ERR_LOG("socket");
		return -1;
	}
	int reuse =1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int))<0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}

	struct sockaddr_in sin;//绑定服务器的IP和端口，填充地址信息结构体
	sin.sin_family=AF_INET;
	sin.sin_port=htons(PORT);//端口号的网络字节序
	sin.sin_addr.s_addr=inet_addr(IP);//IP地址的网络字节序

	if(connect(sfd,(struct sockaddr*)&sin,sizeof(sin))<0)//连接服务器
	{
		ERR_MSG("connect");
		return -1;
	}
	

	char a;
PP:
PL:
	while(1)
	{
		system("clear");
		printf("***********电子词典*************\n");
		printf("***********1.注册***************\n");
		printf("***********2.登录***************\n");
		printf("***********5.退出***************\n");
		printf("********************************\n");
		printf("input>>>>");
		scanf("%c",&a);
		while(getchar()!=10);
		switch(a)
		{
			case '1'://注册
				do_regis(sfd);
				goto PL;
				break;
			case '2'://登录
				do_login(sfd);
				goto PP;
				break;
			case '5'://退出
				return 0;
				break;
			default:
				printf("您的输入不合法\n");
		}
	}
	close(sfd);
	return 0;
}

int do_login(int sfd)
{
	msg log;
	log.type='2';
	size_t size=0;
	char buf[sizeof(log)];

	while(1)
	{
		printf("请输入账号>>>>>\n");
		scanf("%s",log.name);
		while(getchar()!=10);
		printf("请输入密码>>>>>\n");
		scanf("%s",log.date);
		while(getchar()!=10);
		if(send(sfd,(void *)&log,sizeof(log),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}


		size=recv(sfd,buf,sizeof(buf),0);//接收服务器的数据
		if(size<0){
			ERR_MSG("recv");
			return -1;
		}
		log=*(msg *)buf;
		if(log.type=='6')
		{
			printf("您的账号密码输入有误,请重新输入\n");
			while(getchar()!=10);
			return 0;
		}
		else if(log.type=='7')
		{
			do_input(sfd);
			return 0;
		}
	}
	return 0;
}


int do_list(int sfd)
{
	msg log;
	log.type='3';
	size_t size=0;
	char buf[sizeof(log)];

	while(1)
	{
		printf("请输入单词>>>>>\n");
		scanf("%s",log.name);
		while(getchar()!=10);
		if(send(sfd,(void *)&log,sizeof(log),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}
XX:		
		size=recv(sfd,buf,sizeof(buf),0);//接收服务器的数据
		if(size<0)
		{
			ERR_MSG("recv");
			return -1;
		}
		log=*(msg *)buf;
		if(log.type=='6')
		{
			printf("您输入的单词不存在或者不在题词库中\n");
			while(getchar()!=10);	
			return 0;
		}
		else if(log.type=='7')
		{
			printf("%s : %s \n",log.name,log.date);
			goto XX;
		}
		else if(log.type=='5')
		{
			printf("查询成功\n");
			while(getchar()!=10);	
			return 0;
		}
	}
	return 0;
}

int do_list4(int sfd)
{
	msg log;
	log.type='4';
	size_t size=0;
	int i=0;
	char buf[sizeof(log)];

	while(1)
	{
		if(send(sfd,(void *)&log,sizeof(log),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}
XX:		
		size=recv(sfd,buf,sizeof(buf),0);//接收服务器的数据
		if(size<0)
		{
			ERR_MSG("recv");
			return -1;
		}
		log=*(msg *)buf;
		
	
		if(log.type=='4')
		{
			i++;
			if(i%3==0)
			{
				printf("%20s\n",log.name);
			}else
			{
				printf("%20s",log.name);
			}
			
			goto XX;
		}
		else if(log.type=='5')
		{
			printf("查询成功\n");
			while(getchar()!=10);	
			return 0;
		}
	}
	return 0;
}


int do_quit(int sfd)
{
	msg log;
	log.type='5';
	if(send(sfd,(void *)&log,sizeof(log),0)<0)//发送退出
	{
		ERR_MSG("send");
		return -1;
	}
}

int do_input(int sfd)
{
	sighandler_t s = signal(2, handler); 
	if(SIG_ERR == s) 
	{
		perror("signal"); 
		return -1; 
	}
	char a;
	while(1)
	{
		system("clear");
		printf("***********电子词典*************\n");
		printf("***********3.查单词*************\n");
		printf("***********4.历史记录***********\n");
		printf("***********5.退出***************\n");
		printf("********************************\n");
		printf("input>>>>");
		scanf("%c",&a);
		while(getchar()!=10);
		switch(a)
		{
			case '3':
				do_list(sfd);
				break;
			case '4':
				do_list4(sfd);
				break;
			case '5'://退出
				do_quit(sfd);
				return 0;
				break;
			default:
				printf("您的输入不合法\n");
		}
	}

	return 0;
}



int do_regis(int sfd)
{	
	msg log;
	log.type='1';
	size_t size=0;
	char buf[sizeof(log)];

	while(1)
	{
		printf("请输入账号>>>>>");
		scanf("%s",log.name);
		while(getchar()!=10);
		printf("请输入密码>>>>>");
		scanf("%s",log.date);
		while(getchar()!=10);
		if(send(sfd,(void *)&log,sizeof(log),0)<0)//发送注册
		{
			ERR_MSG("send");
			return -1;
		}
		size=recv(sfd,buf,sizeof(buf),0);//接收服务器的数据
		if(size<0){
			ERR_MSG("recv");
			return -1;
		}
		log=*(msg *)buf;
		if(log.type=='6')
		{
			printf("您的账号已存在,请重新输入\n输入任意字符进行下一步\n");
			while(getchar()!=10);
			return -1;
		}
		else if(log.type=='7')
		{
			printf("注册成功\n输入任意字符进行下一步\n");
			while(getchar()!=10);
		}
		return 0;
	}

}

