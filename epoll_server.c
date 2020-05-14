#include "epoll_server.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#define MAXSIZE 1024



int init_listen_fd(int port, int epfd)
{
	// 创建监听的套接字
	int lfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	// 绑定本地 ip 和端口
	struct sockaddr_in serv;
	
	//清空
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(port);
	
	//端口复用
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 	
	//绑定
	Bind(lfd, (struct sockaddr*)&serv, sizeof(serv));
	
	//设置监听
	Listen(lfd, 128);
	
	// lfd 添加到 epoll 树上
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
		perr_exit("epoll_ctl failed");
	
	
	return lfd;
}

/*接受新连接请求*/
void do_accept(int lfd, int epfd)
{
	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	int cfd = Accept(lfd, (struct sockaddr*)&client, &len);
	
	
	//打印客户端信息
	char ip[64] = {0};
	printf("New Client Ip: %s, Port: %d, cfd = %d\n",
			inet_ntop(AF_INET, &client.sin_addr.s_addr, ip, sizeof(ip)),
			ntohs(client.sin_port), cfd);
			
	//接受请求后，的到新的节点，上树
	struct epoll_event ev;
	
	
	//设置cfd为非阻塞
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);
	
	//边缘非阻塞
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = cfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
		perr_exit("epoll_ctl add cfd failed");
	
	
	
}


/*解析http请求消息的每一行内容*/
int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;
	while((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
					
			}
			buf[i] = c;
			i ++;
			
		}
		else
			c = '\n';
	}
	
	buf[i] = '\0';
	
	
	if (n == -1)
		i = -1;
	
	return(i);
}

/*关闭套接字，cfd 从 epoll 上 del*/
void disconnect(int cfd, int epfd)
{
	int ret =  epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret == -1)
	{
		perr_exit("epoll_ctl del cfd falied!");
	}
	close(cfd);
}

/*读数据*/
void do_read(int cfd, int epfd)
{
	//将浏览器发过来的数据读取到buf中
	char line[1024] = {0};
	
	//读请求行
	int len = get_line(cfd, line, sizeof(line));
	
	if (len == 0)
	{
		printf("客户端断开连接 \n");
		
		// 关闭套接字，cfd 从 epoll 上 del
		disconnect(cfd, epfd);
		
	}
	else if (len == -1)
	{
		//关闭套接字， cfd 从 epoll 上 del
		perror("recv error");
		exit(1);
	}
	else
	{
		printf("请求行数据：%s", line);
		//还有数据继续读
		
		while(len)
		{	
			char buf[1024] = {0};
			len = get_line(cfd, buf, sizeof(buf));
			printf("=====请求头=====\n");
			printf("-----: %s", buf);
			
		}
	}
	
	//请求行：get /xxx http/1.1
	//判断是不是get请求
	if (strncasecmp("get", line, 3) == 0)
	{
		// 处理http请求
		http_request(line, cfd);
		
	}
	
}

/*发送消息报头*/
void send_respond_head(int cfd, int no, const char* desp, const char* type, long len)
{
	char buf[2014] = {0};
	//状态行
	sprintf(buf, "http/1.1 %d %s \r\n", no, desp);
	send(cfd, buf, strlen(buf), 0);
	
	
	//消息报头
	sprintf(buf, "Content-Type: %s \r\n", type);
	//send(cfd, buf, strlen(buf), 0);
	sprintf(buf + strlen(buf), "Content-Lenth: %ld \r\n", len);
	send(cfd, buf, strlen(buf), 0);
	
	//空行
	send(cfd, "\r\n", 2, 0);
}

/*发送文件内容*/
void send_file(int cfd, const char* filename)
{
	// 打开文件
	int fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		//show 404
		return;
	}
	//循环读文件
	char buf[4096] = {0};
	int len = 0;
	while ((len = read(fd, buf, sizeof(buf))) > 0)
	{
		//发送读出的数据
		send(cfd, buf, len, 0);
	}
	if (len == -1)
		perr_exit("read file error！");
	
	close(fd);
	
}


void send_dir(int cfd, const char* dirname)
{
	//拼一个html的页面<table></table>
	char buf[4096] = {0};
	sprintf(buf, "<html><head><title>目录名：%s </title></head>", dirname);
	sprintf(buf+strlen(buf), "<body><h1>当前目录：%s </h1><table>", dirname);
	
	
	/*
	// 递归读文件拼接
	DIR* dir = opendir(dirname);
	if (dir = NULL)
	{
		perr_exit("opendir error");
	}
	//读目录
	struct dirent* ptr == NULL;
	whlie((ptr = readdir(dir)) != NULL )
	{	
		sprintf(buf, "<tr><td></td>")
		char* name = ptr->d_name;
		
		
	}
	
	
	closedir(dir);
	*/
	
	char path[1024] = {0};
	// 目录项二级指针
	struct dirent** ptr;
	int num = scandir(dirname, &ptr, NULL, alphasort);
	//遍历文件数组
	int i;
	for (i = 0; i < num; i++)
	{
		char* name = ptr[i]->d_name;
		//拼接目录完整路径
		sprintf(path, "%s/%s", dirname, name);
		struct stat st;
		stat(path, &st);
		//如果是文件
		if (S_ISREG(st.st_mode))
		{
			sprintf(buf+strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", dirname, dirname, (long)st.st_size);
		}
		else if (S_ISDIR(st.st_mode))
		{
			sprintf(buf+strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", dirname, dirname, (long)st.st_size);
		}
		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));
		//字符串拼接
		
	}
	
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	printf("dir message send ok!");
	
}


void http_request(const char* request, int cfd)
{
	//拆分http请求行
	char method[10], path[1024], protocol[12];
	sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
	printf("method = %s, path = %s, protocol = %s \n", method, path, protocol);
	
	//处理path去除/
	char *file = path + 1;
	
	//如果没有指定访问资源，默认显示资源目录的内容
	if (strcmp(path, "/") == 0)
	{
		//file 的值
		file = "./";
	}
	
	
	//获取文件的属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		//show 404
		perr_exit("stat error");
	}
	//判断是否是目录还是文件
	if (S_ISDIR(st.st_mode))
	{
		//发送头信息
		send_respond_head(cfd, 200, "OK", "text/html", -1);
		
		
		//发送目录的函数
		send_dir(cfd, file);
		
	}
	else if (S_ISREG(st.st_mode))
	{
		// 文件
		//发送消息报头
		send_respond_head(cfd, 200, "OK", "text/plain", st.st_size);
		
		//发送文件内容
		send_file(cfd, file);
	}
}



void epoll_run(int port)
{
	int i;
	
	// 创建一个epoll树的根节点
	int epfd = epoll_create(MAXSIZE);
	if (epfd == -1)
		perr_exit("epoll_create failed");
	
	//添加要监听的节点
	int lfd = init_listen_fd(port, epfd);
	
	//检测添加到树上的节点
	struct epoll_event all[MAXSIZE];
	while (1)
	{
		int ret = epoll_wait(epfd, all, MAXSIZE, -1);
		if (ret == -1)
			perr_exit("epoll_wait failed");
		
		// 遍历发生变化的节点
		for (i = 0; i < ret; i++)
		{
			//只处理读事件，其他事件不处理
			struct epoll_event *pev = &all[i];
			if (!(pev->events & EPOLLIN))
				continue;
			
			
			if (pev->data.fd == lfd)
			{
				//接受连接请求
				do_accept(lfd, epfd);
			}
			else
			{
				//读数据
				do_read(pev->data.fd, epfd);
			}
		}
	}
	
}
