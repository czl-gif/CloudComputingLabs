#ifndef HTTPSERVER_H_INCLUDE
#define HTTPSERVER_H_INCLUDE


#include <sys/socket.h>

extern int server_socket;//服务器套接字
extern char* ip;
extern int port;
extern int number_thread;
#define backlog 5000

extern char proxy[255];
//初始化数据以及输入参数解析
extern void init(int argc, char * const argv[]);
//启动服务器
extern void Start();
#endif