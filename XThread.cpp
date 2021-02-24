#include "XThread.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<string.h>
#include<string>
#include <unistd.h>
#include <sys/stat.h>
#include<dirent.h>
#include <time.h>
#include "md5.h"

using namespace std;

XThread::XThread(){}

XThread::~XThread(){}


void XThread::main(){
	{
		char buf[1024] = { 0 };

		for (;;){
			int len = recv(this->cfd,buf,sizeof(buf),0);

			if (len < 1){
				break;
			}


			if (!this->getHead(buf)){
				break;
			}

			
			struct stat fileStat;
			char *filePath = this->getFile;
			
			if (stat(filePath, &fileStat) < 0){
				
				printf("file not found\n");
				this->sendHeader(404,"NOT FOUND",this->getMimeType("*.html").c_str(),0);
				this->sendFile("error.html");
			}
			else
			{

				if (S_ISDIR(fileStat.st_mode)){

					printf("is dir\n");
					
					this->sendHeader(200, "OK", this->getMimeType("*.html").c_str(),0);
					
					
					struct dirent **list = NULL;
					
					int dirLen = 0;

					int dirSize = scandir(filePath,&list,NULL,alphasort);
					
					char dirBuf[512] = "";
					int i = 0;
					for (; i < dirSize; i++){
						
						if (list[i]->d_type == DT_DIR){
							dirLen = sprintf(dirBuf, "<a href='%s/'>%s</a><br />", list[i]->d_name, list[i]->d_name);
						}
						else{
							dirLen = sprintf(dirBuf, "<a href='%s'>%s</a><br />", list[i]->d_name, list[i]->d_name);
							
						}
						
						send(this->cfd, dirBuf, dirLen, 0);

						free(list[i]);
					}
					
					free(list);
					

				}
				else if (S_ISREG(fileStat.st_mode)){

					printf("is_reg\n");

					if (this->isPhp){
					
						char phpFile[256];
						strcpy(phpFile, this->htmlPath);
						strcat(phpFile, "/");
						strcat(phpFile, filePath);


						string cmd = "php-cgi -q ";
						cmd += phpFile;

						if (this->getQuery != ""){
							cmd += " "+this->getQuery;
						}

						string tmpPhpName = this->getTmpName();

						cmd += " > " + tmpPhpName;

						system(cmd.c_str());

						char phpTmp[256];
						strcpy(phpTmp, tmpPhpName.c_str());

						if (stat(phpTmp, &fileStat) < 0){

							printf("file not found\n");
							this->sendHeader(404, "NOT FOUND", this->getMimeType("*.html").c_str(), 0);
							this->sendFile("error.html");

						}
						else{
						
							this->sendHeader(200, "OK", this->getMimeType("*.html").c_str(), fileStat.st_size);
							this->sendFile(phpTmp);
						}


					}else{
						this->sendHeader(200, "OK", this->getMimeType(filePath).c_str(), fileStat.st_size);
						this->sendFile(filePath);

					}


					


				}


			}


		}
		close(this->cfd);

		delete this;
	}

}



string XThread::getMimeType(const char *name){

	const char* dot;
	dot = strrchr(name,'.');

	if (dot == (char*)0){
		return "text/plain; charset=utf-8";

	}

	if(strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
		return "text/html; charset=utf-8";
	}


	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) {
		return "image/jpeg";
	}

	if (strcmp(dot, ".gif") == 0) {
		return "image/gif";
	}

	if (strcmp(dot, ".png") == 0) {
		return "image/png";
	}

	if (strcmp(dot, ".css") == 0) {
		return "text/css";
	}

	if (strcmp(dot, ".mp3") == 0) {
		return "audio/mpeg";
	}

	if (strcmp(dot, ".ogg") == 0) {
		return "application/ogg";
	}

	return "text/plain; charset=utf-8";
	
}

//读取第一行，获取请求方式，要请求的文件
bool XThread::getHead(char buf[1024]){

	char method[256] = "";
	char content[256] = "";
	char protocol[256] = "";
	sscanf(buf,"%[^ ] %[^ ] %[^ \r\n]", method, content, protocol);
	if (strcmp(method, "GET") != 0 || (strcmp(protocol, "HTTP/1.1") != 0 && strcmp(protocol, "HTTP/1.0") != 0)){
		return false;
	}
	char file[128] = "";
	char query[128] = "";
	sscanf(content, "/%[^ ?]%[^ ]", file, query);

	this->strDecode(file, file);

	string queryStr = "";
	if (strlen(query) > 0 && query[0] == '?'){
		for (int i = 1; i < strlen(query); i++){

			string tmp;
			if (query[i] == '&'){
				tmp = " ";
			}else{
				tmp = string(1, query[i]);
			}

			queryStr += tmp;
		}
	}


	string last;
	for (int i = 0; i < strlen(file); i++){
		if (file[i] == '.'){
			last = "";
		}else{
			last += string(1,::tolower(file[i]));
		}

	}

	if (last == "php"){
		this->isPhp = true;
	}

	if (strcmp(file,"") == 0){
		strcpy(file, "./");
	}
	strcpy(this->getFile, file);

	this->getQuery = queryStr;

	return true;


}


//向浏览器发送标准头
void XThread::sendHeader(int code, const char *info, const char *fileType, int length){

	char buf[512];
	int len = 0;

	len = sprintf(buf, "HTTP/1.1 %d %s\r\n", code, info);

	send(this->cfd, buf, len, 0);
	
	
	len = sprintf(buf, "Content-Type:%s\r\n", fileType);
	send(this->cfd, buf, len, 0);
	
	if (length > 0) {
		len = sprintf(buf, "Content-Length:%d\r\n", length);
		send(this->cfd, buf, len, 0);
	}
	send(this->cfd, "Power-by:XHttp\r\n", 16, 0);
	send(this->cfd, "\r\n", 2, 0);
	
	return;

}


void XThread::sendFile(const char *path){
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return;
	}

	
	char buf[1024] = "";
	int len = 0;
	while (1)
	{

		len = read(fd, buf, sizeof(buf));
		if (len < 0){
			perror("read");
			break;
		}
		else if (len == 0){
			break;
		}
		else{
			send(this->cfd, buf, len, 0);
		}

	}

	close(fd);
	return;

}


string XThread::getTmpName(){

	struct timespec tm;

	clock_gettime(CLOCK_REALTIME, &tm);

	srand((unsigned)time(NULL));
	

	int aTmp[3] = { rand() % 100 + 1, rand() % 1000 + 1, rand() % 10000 + 1 };

	int key = rand() % 3;

	string tmp = MD5(::to_string(aTmp[key])).toStr() + MD5(::to_string(tm.tv_nsec)).toStr();

	return "/tmp/"+MD5(tmp).toStr();


}

void XThread::strDecode(char *to, char *from){
	for (; *from != '\0'; ++to, ++from) {

		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) { 
			*to = this->hexit(from[1]) * 16 + this->hexit(from[2]);
			from += 2;                      
		}
		else{
			*to = *from;
		}
			
	}
	*to = '\0';
}

int XThread::hexit(char c){
	if (c >= '0' && c <= '9'){
		return c - '0';
	}
		
	if (c >= 'a' && c <= 'f'){
		return c - 'a' + 10;
	}
		
	if (c >= 'A' && c <= 'F'){
		return c - 'A' + 10;
	}
	
	return 0;
}


