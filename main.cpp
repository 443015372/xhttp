#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "XThread.h"
#include <fcntl.h>
#include<string>
#include<thread>
#include <unistd.h>
#include<signal.h>
#include<string.h>
using namespace std;


int main(int argc, char *argv[]){


	signal(SIGPIPE, SIG_IGN);



	char path[256] = "";
	char *pwd = getenv("PWD");
	strcpy(path, pwd);
	strcat(path, "/html");
	chdir(path);




	unsigned short port = 80;

	if (argc > 1){
		port = atoi(argv[1]);
	}


	//创建socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);


	//绑定端口
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;


	//设置端口复用
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (::bind(sock, (sockaddr *)&addr, sizeof(addr)) != 0){
		perror("bind");
		return -2;
	}

	printf("bind %d success\n", port);

	//监听
	listen(sock, 128);

	
	//循环提取套接字交给线程处理
	for (;;){

		sockaddr_in clientAddr;
		socklen_t clientLen = sizeof(clientAddr);

		int cfd = accept(sock, (sockaddr *)&clientAddr, &clientLen);

		if (cfd <= 0){
			break;
		}


		
		int flag = fcntl(cfd, F_GETFL);
		flag |= O_NONBLOCK;
		fcntl(cfd, F_SETFL, flag);

		
		//创建子线程
		XThread *th = new XThread();
		th->cfd = cfd;
		strcpy(th->htmlPath, path);
		thread sth(&XThread::main, th);
		sth.detach();
		
	}


	close(sock);

	return 0;

}