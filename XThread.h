#pragma once
#include<string>

using namespace std;
class XThread
{
public:
	XThread();
	~XThread();

	void main();

	bool getHead(char buf[1024]);

	void strDecode(char *to, char *from);

	int hexit(char c);

	void sendHeader(int code, const char *info, const char *fileType, int length);

	void sendFile(const char *path);

	//char *getMimeType(char *name);

	string getMimeType(const char *name);
	string getTmpName();


	int cfd = 0;
	bool isPhp = false;

	char htmlPath[256];

	//请求的文件
	char getFile[512];

	//php 的get 参数
	string getQuery;

};

