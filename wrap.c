#include "wrap.h"

void perr_exit(const char *s)
{
	perror(s);
	exit(-1);
}



int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int n;

again:
	if ((n = accept(sockfd, addr, addrlen)) < 0)
	{
		//如果是被信号中断和软件层次中断， 不能退出，重新建立连接
		if ((errno == ECONNABORTED) || (errno == EINTR))
			goto again;
		else
			perr_exit("accept error");
	}

	return n;
}


int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int n;
	if ((n = bind(sockfd, addr, addrlen)) < 0)
	{
		perr_exit("bind error");
	}
	return n;
}


int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int n;
	if ((n = connect(sockfd, addr, addrlen)) < 0)
	{
		perr_exit("connect error");
	}
	return n;
}


int Listen(int sockfd, int backlog)
{
	int n;
	if ((n = listen(sockfd, backlog)) < 0)
	{
		perr_exit("listen error");
	}
	return n;
}



int Socket(int domain, int type, int protocol)
{
	int n;
	if ((n = socket(domain, type, protocol)) < 0)
	{
		perr_exit("socket error");
	}
	return n;
}


ssize_t Read(int fd, void *buf, size_t count)
{
	ssize_t n;

again:
	if ((n == read(fd, buf, count)) == -1)
	{
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}



ssize_t Write(int fd, void *buf, size_t count)
{
	ssize_t n;

again:
	if ((n == write(fd, buf, count)) == -1)
	{
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}



int Close(int fd)
{
	int n;
	if ((n = close(fd)) == -1)
	{
		perr_exit("close error");
	}

	return n;
}


ssize_t Readn(int fd, void *buf, size_t count)
{
	size_t nleft; // usigned int 剩余未读取的字节数
	ssize_t nread; // int 实际读取的字节数
	char *ptr;
	
	ptr = buf;
	nleft = count;

	while(nleft > 0)
	{
		if ((nread == read(fd, ptr, nleft)) < 0)
		{
			if (nread == EINTR)
				nread = 0;
			else 
				return -1;

		}
		else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return count - nleft;
}

ssize_t Writen(int fd, void *buf, size_t count)
{
	size_t nleft; // usigned int 剩余未写的字节数
	ssize_t nwritten; // int 实际读写的字节数
	char *ptr;
	
	ptr = buf;
	nleft = count;

	while(nleft > 0)
	{
		if ((nwritten == write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && errno == EINTR)

				nwritten = 0;
			else 
				return -1;

		}
		

		nleft -= nwritten;
		ptr += nwritten;
	}
	return count;
}


int tcp4bind(short port, const char *ip)
{
	struct sockaddr_in serv_addr;
	int lfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&serv_addr, sizeof(serv_addr));

	if (ip == NULL)
		serv_addr.sin_addr.s_addr = INADDR_ANY;
	else
		{
			if (inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr) <= 0)
			{
				perror(ip);
				exit(1);
			}
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		
		//端口复用
		int opt = 1;
		setsockopt(lfd,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		return lfd; 
}
