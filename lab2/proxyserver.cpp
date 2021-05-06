#include "httpserver.h"
#include "proxyserver.h"
#include <iostream>
#include <string>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <map>
#include "threadpool.h"
using namespace std;
map<int, int> socket_map;
char *UpstreamIP = new char[45];
int UpstreamPort;
int epfd;
static void addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, F_GETFD, 0) | O_NONBLOCK);
    //printf("fd added to epoll!\n\n");
}

static void delfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    //printf("fd deleted to epoll!\n\n");
}

int find_http(int n)
{
    int res = 0;
    for(int i = 0; i < n; i++) {
        if (proxy[i] == '/')
            res = i + 1;
    }
    return res;
}
void proxyinit()
{
    int n = strlen(proxy);
    int  P = 0;
    bool dm = false;
    int index = find_http(n);
    for(int i = index; i < n; i ++) {
        if (!dm && ((proxy[i] >= 'a' && proxy[i] <= 'z') || (proxy[i] >= 'A' && proxy[i] <= 'Z')))
            dm = true;
        if (!P && proxy[i] == ':')
            P = i + 1;
        if (P && dm)
            break;
    }
    if (P != 0) {
        UpstreamPort = atoi(proxy + P);
    }else {
        UpstreamPort = 80;
    }
    if(P == 0)
        P = n;
    else P --;
    int t = 0;
    char usip[255];
    for (int i = index; i < P; i++) {
        usip[t++] = proxy[i];
    }
    if (dm) {
        struct hostent *h;
        if((h=gethostbyname(usip))==NULL) {
            perror("域名错误");
        }
        strcpy(UpstreamIP, inet_ntoa(*((struct in_addr *)h->h_addr_list[0])));
    }
    else {
        strcpy(UpstreamIP, usip);
    }
    
    epfd = epoll_create(backlog);
    if(epfd < 0) {
        perror("epfd error");
        exit(-1);
    }
}

void forward_(int from_socket, int to_socket)
{
    char *msg = new char[65536];
    int len = recv(from_socket, msg, 65536 * sizeof(char), 0);
    if (len <= 0) {
        socket_map[from_socket] = 0;
        socket_map[to_socket] = 0;
        close(from_socket);
        close(to_socket);
        return ;
    }
    send(to_socket, msg, 65536 * sizeof(char) , 0);
}

static struct epoll_event events[backlog];
void proxyStart()
{
    proxyinit();
    cout << "上游IP = " << UpstreamIP << ":" << UpstreamPort << endl;
    struct sockaddr_in serv_addr;
    struct sockaddr_in remote_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&remote_addr, 0, sizeof(remote_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    if(bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind error!");
        exit(1);
    }
    if(listen(server_socket, backlog) == -1) {
        perror("listen error!");
        exit(1);
    }
    addfd(epfd, server_socket, true);
    socklen_t sin_size = sizeof(struct sockaddr_in);
    threadpool executor(number_thread);
    while(1) {
        int epoll_events_count = epoll_wait(epfd, events, backlog, -1);
        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }
        for(int i = 0; i < epoll_events_count; i++) {
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == server_socket) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(server_socket, (struct sockaddr*)&client_address, &client_addrLength);
                addfd(epfd, clientfd, true);
                int client_sockfd;
	            int len;
                memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
	            remote_addr.sin_family=AF_INET; //设置为IP通信
	            remote_addr.sin_addr.s_addr=inet_addr(UpstreamIP);//服务器IP地址
	            remote_addr.sin_port=htons(UpstreamPort); //服务器端口号
	
	            /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
	            if((client_sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	            {
		            perror("socket error");
		            exit(1);
	            }
	
	            /*将套接字绑定到服务器的网络地址上*/
	            if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	            {
		            perror("connect error");
		            exit(1);
	            }
                addfd(epfd, client_sockfd, true);
                socket_map[clientfd] = client_sockfd;
                socket_map[client_sockfd] = clientfd;
            }else {
                executor.commit(forward_, sockfd, socket_map[sockfd]);
            }
        }
    }
}