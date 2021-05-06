#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
using namespace std;
int main(int argc, char *argv[])
{
	int client_sockfd;
	int len;
	struct sockaddr_in remote_addr; //服务器端网络地址结构体
	//char buf[BUFSIZ];  //数据传送的缓冲区
	memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
	remote_addr.sin_family=AF_INET; //设置为IP通信
	remote_addr.sin_addr.s_addr=inet_addr("127.0.0.1");//服务器IP地址
	remote_addr.sin_port=htons(8888); //服务器端口号
	
	/*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
	if((client_sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket error");
		return 1;
	}
	
	/*将套接字绑定到服务器的网络地址上*/
	if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)
	{
		perror("connect error");
		return 1;
	}
	printf("connected to server\n");
	//len=recv(client_sockfd,buf,BUFSIZ,0);//接收服务器端信息
    //    buf[len]='/0';
	//printf("%s",buf); //打印服务器端信息
	
	/*循环的发送接收信息并打印接收信息（可以按需发送）--recv返回接收到的字节数，send返回发送的字节数*/
	
	{
		printf("Enter string to send:");
		//scanf("%s",buf);
        string s = "GET / HTTP/1.0\r\nhost: www.baidu.com\r\n\r\nGET /index.html HTTP/1.0\r\nhost: www.baidu.com\r\n";
        //s += "GET / HTTP/1.1\r\nhost: www.baidu.com\r\n";
        const char* buf = new char[128];
        buf = s.c_str();
		cout << buf << endl;
        cout << strlen(buf) << endl;
		len=send(client_sockfd,s.c_str(),s.length(),0);
		//len=recv(client_sockfd,buf,BUFSIZ,0);
		//buf[len]='/0';
		//printf("received:%s\n",buf);
        char* buf0 = new char[65536];
        while((len = recv(client_sockfd, buf0, 65536, 0)) > 0) {
            cout << "len = " << len << endl;
            cout << buf0 << endl;
            memset(buf0,'\0',strlen(buf0));
        }
	}
	
	/*关闭套接字*/
	close(client_sockfd);
    
	return 0;
}
