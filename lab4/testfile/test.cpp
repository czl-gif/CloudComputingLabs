#include <sstream>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
using namespace std;
string get(string value)
{
    string res = "*2\r\n$3\r\nGET\r\n$";
    res += to_string(value.length());
    res += "\r\n";
    res += value;
    res += "\r\n";
    return res; 
}
string DEL(string value)
{
    string res = "*2\r\n$3\r\nDEL\r\n$";
    res += to_string(value.length());
    res += "\r\n";
    res += value;
    res += "\r\n";
    return res; 
}
string set(string key, string value)
{
    int n = 3;
    for(int i = 0; i < value.size(); i++){
        if(value[i] == ' ')
        n++;
    }
    string res = "*" + to_string(n) + "\r\n$3\r\nSET\r\n$";
    res += to_string(key.length());
    res += "\r\n";
    res += key;
    res += "\r\n";
    stringstream ss(value);
    string s;
    while(ss >> s) {
        res += "$";
        res += to_string(s.length());
        res += "\r\n";
        res += s;
        res += "\r\n";
    }
    return res;
}
int main()
{
    int clientfd;
    struct sockaddr_in seraddr;
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(8002);
    seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create client socket error!!!");
        exit(1);
    }
    if ((connect(clientfd, (struct sockaddr*)&seraddr, sizeof(seraddr))) < 0)
    {
        perror("connect error");
        exit(1);
    };
    //string buf = set("cs","jh lkk");
    //string buf = get("cs lk");
    string s1, s2, s3;
    int self_socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);
    int flag = 1;
    if (setsockopt(self_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
        perror("set coordinator_socket error!");
        exit(1);
    }
    if(bind(self_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("coordinator_socket bind error!");
        exit(1);
    }
    if(listen(self_socket,10) == -1) {
        perror("coordinator_socket listen error!");
        exit(1);
    }

    while(1) {
        cin>> s1;
        cout << "s1 = " << s1 << endl;
        string buf;// = "*3\r\n$3\r\nDEL\r\n$7\r\nCS06142\r\n$5\r\nCS162\r\n";
        if(s1 == "GET") {
            cin >> s2;
            buf = get(s2);
        }else if(s1  == "SET") {
            cin >> s2;
            cin.get();
            getline(cin, s3);
            buf = set(s2, s3);
            cout << "buf = " << buf << endl;
        }else if(s1 == "DEL") {
            cin >> s2;
            buf = DEL(s2);
            cout << "buf = " << buf << endl;
        }
        if (send(clientfd, buf.c_str(), buf.length(), 0) < 0)
        {
            perror("send error");
        }
        char buf0[255];
        close(clientfd);
        socklen_t sin_size = sizeof(struct sockaddr_in);
        clientfd = accept(self_socket, (struct sockaddr*)&serv_addr, &sin_size);
        memset(buf0, 0, 255);
        read(clientfd, buf0, 255);
        cout << "read from :" << buf0 << endl;
    }
    
    close(clientfd);
}