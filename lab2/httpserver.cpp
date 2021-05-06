#include "httpserver.h"
#include "threadpool.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fstream>
using namespace std;

int server_socket;
char *ip = new char[45];
int port = 0;
int number_thread = 0;
char proxy[255];
//int backlog = 100; //服务器监听请求缓存大小
threadpool* executor;

void init(int argc, char * const argv[])
{
    server_socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
    int opt;
    int option_index = 0;
    //char *string = ":proxy";
    char *string = new char[2];
    static struct option long_options[]{
        {"ip", required_argument, NULL, 1},
        {"port", required_argument, NULL, 2},
        {"number-thread", required_argument, NULL, 3},
        {"proxy", required_argument, NULL, 4},
    };
    while ((opt = getopt_long(argc, argv, string, long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 1:
            strcpy(ip, optarg);
            //cout << "ip0 = " << ip << endl;
            break;
        case 2:
            port = atoi(optarg); //(*optarg - '0') << 3  ;
            //cout << "port0 = " << port <<" " << *optarg << endl;
            break;
        case 3:
            number_thread = atoi(optarg);
            break;
        case 4:
            strcpy(proxy, optarg);
            break;
        default:
            break;
        }
    }
    delete []string;
}
//404NotFound body
string NotFound()
{
    string res = "<html><title>404 Not Found</title><body bgcolor=ffffff>\r\n";
    res += "Not Found\r\n<hr><em>HTTP Web server</em>\r\n";
    res += "</body></html>\r\n";
    return res;
}
//寻找下一个CRLF的后位置，如果字符串查找完，返回长度
int find_r_n(char *messages, int index, int len)
{
    if (index >= len)
        return len;
    while(index + 1 < len) {
        if (messages[index] == '\r' && messages[index + 1] == '\n') 
            return index + 2;
        index++;
    }
    return len;
}
//获取一行
string Getline(char *msg, int l, int r)
{
    string res;
    int space = 0;
    for(int i = l; i < r - 1; i++) {
        if(msg[i] == ' ')
            space++;
        res += msg[i];
    }
    if (space != 2) 
        return "";
    return res;
    
}

//获取响应报文的头部行
string response_header(string status, int length)
{
    string res = "HTTP/1.1 ";
    res += status;
    res += "\r\n";
    res += "Server: Chen Guo Global Support Association's Web Server\r\n";
    res += "Content-length: ";
    res += to_string(length);
    res +="\r\nContent-type: text/html\r\nConnection: keep-alive\r\n\r\n";
    //cout << "报文头=" << res << endl;
    return res;
}
//发送文件
void SendFile(int conn_sock, string url)
{
    /*fstream file;
    file.open(url, ios::in | ios::binary);
    while(!file.eof()) {
        char* msg = new char[65536];
        file.read(msg, 65536);
        if (send(conn_sock, msg, strlen(msg), 0) < 0) {             
            perror("file send error!");                   
            exit(1);
        }
        cout << "发送内容：：" << endl;
        cout << msg << endl;
        delete []msg;
    }*/
    FILE* fq;
    fq = fopen(url.c_str(), "rb");
    char buffer[4096];
    bzero(buffer,sizeof(buffer));
    while(!feof(fq)){
        int len = fread(buffer, 1, sizeof(buffer), fq);
        if(len != write(conn_sock, buffer, len)){
            printf("write.\n");
            break;
        }
    }
}
//处理get方法
void get_handle(int conn_sock, string url)
{
    fstream file;
    struct stat s;
    
    if(url.length() != 0 && stat(url.c_str(), &s) == 0 && (s.st_mode & S_IFREG)) {
        file.open(url, ios::in);
        if (!file) {
            cout << url << "文件不存在！" << endl;
            file.close(); 
        }else {
            file.seekg(0, ios::end);
            int length = file.tellg();
            file.close();
            string res = response_header("200 OK", length);
            if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
                perror("header send error!");
                exit(1);
            }
            SendFile(conn_sock, url);
            /*if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
                perror("header send error!");
                exit(1);
            }*/
            return;
        }
    }
    url += "index.html";
    if (stat(url.c_str(), &s) != 0 || !(s.st_mode & S_IFREG)) {
        cout << url << "文件也不存在！" << endl;
        string body = NotFound();
        string res = response_header("404 Not Found", body.length()); 
        if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
            perror("header send error!");
            exit(1);
        }
        if (send(conn_sock, body.c_str(), body.length(), 0) <= 0) {
            perror("GET body send error!");
            exit(1);
        }
        //cout << res << body << endl;
    }else {
        file.open(url, ios::in);
        file.seekg(0, ios::end);
        int length = file.tellg();
        file.close();
        string res = response_header("200 OK", length);
        if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
            perror("header send error!");
            exit(1);
        }
        SendFile(conn_sock, url);
    }
    return;
}
void post_handle(int conn_sock, string url, string Name, string ID)
{
    if (url != "/Post_show") {
        string body = NotFound();
        string res = response_header("404 Not Found", body.length()); 
        if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
            perror("header send error!");
            exit(1);
        }
        if (send(conn_sock, body.c_str(), body.length(), 0) <= 0) {
        perror("POST body send error!");
        exit(1);
        return ;
        }
    }
    string body = "<html><title>POST method</title><body bgcolor=ffffff>\r\n";
    body += "Your Name: "; body += Name; body += "\r\nID: ";
    body += ID; body += "\r\n<hr><em>HTTP Web server</em></body></html>\r\n";
    string res = response_header("200 OK", body.length());
    if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
        perror("POST header send error!");
        exit(1);
    }
    if (send(conn_sock, body.c_str(), body.length(), 0) <= 0) {
        perror("POST body send error!");
        exit(1);
    }
}
//提取关键字Name和ID
int GetPostKeys(char* messages, int l, int r, string& Name, string& ID)
{
    if(messages[l] != 'N' || messages[l + 1] != 'a' | messages[l + 2] != 'm'
    | messages[l + 3] != 'e' || messages[l + 4] != '=')
        return 0;
    l += 5;
    while(l < r && messages[l] != '&') {
        Name += messages[l];
        l++;
    }
    if (l >= r || messages[l] != '&')
        return 0;
    l++;
    if (messages[l] != 'I' || messages[l + 1] != 'D' || messages[l + 2] != '=')
        return 0;
    l += 3;
    while(l < r && messages[l] != '\r')
    {
        ID += messages[l];
        l++;
    }
    return 1;
}
void other_handle(int conn_sock, string method)
{
    string body = "<html><title>501 Not Implemented</title><body bgcolor=ffffff>\r\nNot Implemented";
    body += "\r\n<p>Does not implemente this method:";
    body += method;
    body += "\r\n<hr><em>HTTP Web server</em>\r\n</body></html>\r\n";
    string res = response_header("501 Not Implemented", body.length());
    if (send(conn_sock, res.c_str(), res.length(), 0) <= 0) {
        perror("Other method header send error!");
        exit(1);
    }
    if (send(conn_sock, body.c_str(), body.length(), 0) <= 0) {
        perror("Other method body send error!");
        exit(1);
    }
}
void timeout_alarm(int sig)
{
	cout << "time out " << endl;
    
}
//一个socket的总处理函数
void process(int conn_sock)
{
    char *messages = new char[65536];
    //char *msg = new char[65536];
    //bool rest = false;
    while(1) {
        memset(messages, '\0', 65536 * sizeof(char));
        //cout << "连接成功" << endl;
        int len = 0;
        struct timeval tv_out;
        tv_out.tv_sec = 0;
        tv_out.tv_usec = 1;
        setsockopt(conn_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
        //if((len = recv(conn_sock, messages, 65536, 0)) <= 0 && !rest) break;
        if((len = recv(conn_sock, messages, 65536, 0)) <= 0) break;
        /*if(len > 0)
            strncat(messages, msg, len);
        rest = false;
        int len0 = len;
        len = strlen(messages);*/
        //cout << "收到信息:" << messages << endl;
        //cout << "len = " << strlen(messages) << endl;
        int index = 0;//记录字符串处理位置
        
        while(index < len) {
            //cout << "index = " << index << endl;
            int next_index = find_r_n(messages, index, len);
            string line  = Getline(messages, index, next_index);
            //cout << "next_index =" << next_index << endl;
            
            if (line == "") {
                index = next_index;
                continue;
            }
            /*if(len0 > 0 && len - index < 50){
                memset(msg, '\0', 65536 * sizeof(char));
                strncpy(msg, messages + index, len - index);
                memset(messages, '\0', 65536);
                strcpy(messages, msg);
                rest = true;
                break;
            }*/
            index = next_index;
            stringstream ss(line);
            //cout << "line=" << line << endl;
            string method, url, http;
            ss >> method >> url >> http;
        
            http = http.substr(0, 5);
            if (http != "HTTP/") continue;
            if(method == "GET") {            
                //cout << "method =" << method << " url=" << url << endl;
                get_handle(conn_sock, url.substr(1));
            }
            else if(method == "POST") {
                //cout << "method1 =" << method << " url=" << url << endl;
                while((next_index = find_r_n(messages, index, len)) < len 
                && (next_index - index != 2)) {
                    index = next_index;
                }
            
                index = next_index;
                next_index = find_r_n(messages, index, len);
                if (index >= len) {
                    post_handle(conn_sock, "", "", "");
                    continue;
                }
                string Name, ID;
            
                if (GetPostKeys(messages, index, next_index, Name, ID) == 0) {
                    post_handle(conn_sock, "", "", "");
                }else {
                    post_handle(conn_sock, url, Name, ID);
                }        
                index = next_index;
            }else {
                other_handle( conn_sock, method);
            }
        }
        /*cout << "退出" << len0 << endl;
        if (index >= len) {
            memset(messages, '\0', 65536 * sizeof(char));
        }
        if (len0 < 0 && !rest) break;*/
    }
    delete []messages;
    close(conn_sock);
}
void Start()
{
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
    cout << ip <<':' << port << "and" << number_thread << endl;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    executor = new threadpool(number_thread);
    int num = 0;
    while(1)
    {
        
        int conn_sock = accept(server_socket, (struct sockaddr*)&remote_addr, &sin_size);
        if (conn_sock < 0) {
            perror("connect error!");
            exit(1);
        }
        num++;
        executor->commit(process, conn_sock);
        //sleep(1);
        //cout << "task_number = " << num << endl;
        //cout << "rest_num = " << executor->TaskCount() << endl;
        //process(conn_sock);

    }
    
    delete executor;
}