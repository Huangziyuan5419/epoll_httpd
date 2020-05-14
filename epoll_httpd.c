#include "epoll_server.h"


int main(int argc, const char* argv[])
{	
	
	if (argc < 3)
	{	
		printf("eg: ./epoll_httpd port path\n");
		exit(1);
	}
	
	//端口
	int port = atoi(argv[1]);
	
	// 修改进程工作目录
	int ret = chdir(argv[2]);
	if (ret == -1)
	{
		perr_exit("chdir falied!");
	}
	// 启动epoll模型
	epoll_run(port);
	
	
	
	return 0;
}