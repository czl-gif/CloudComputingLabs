#ifndef COORDINATOR_H_INCLUDE
#define COORDINATOR_H_INCLUDE
#include "kvstore2pcsystem.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <sys/epoll.h>
using namespace std;


class coordinator
{
private:
    Info coordinator_info;//存放coordinator的ip+端口号
    vector<Info> participant_info;//存放participant集的ip+端口号
    vector<bool> participant_work;
    set<int> succeed_participant;//存放好的participant的标号(在participant_info中的位置)
    set<int> failed_participant;//存放坏的participant的标号(在participant_info中的位置)
    unordered_map<int, int> connect_participant_pos_socket;//存储与participant成功连接的socket<标号，socket>
    unordered_map<int, int> connect_participant_socket_pos;//存储与participant成功连接的socket<socket,标号>
    int participant_number;//记录participant的个数
    int coordinator_socket;//接收客户端请求的socket
    int backlog;
    int epollfd;//epoll句柄
    Command command;//命令信息
    //绑定接收连接的coordinator_socket;
    void init();
    vector<int> participants_commit_ID;
    void commid_ID_init();
    void participant_work_init();
    //从新连接的socket解析出命令
    int RecvCommand(int conn_socket);
    //心跳检测，2是检测好的participant, 1是检测坏的，3是检测全部, 5是coordinator启动时尝试与participant建立连接
    void Heartbeat_detection(int mode);
    //第一次participant连接失败后尝试再次连接,如果多次都没连接成功，则participant已坏
    static void try_connect(coordinator* cor,int i,int clientfd, struct sockaddr_in seraddr);
    //将socket描述符加入到epollfd，如果enable_et为true，则为水平触发模式。
    void addfd(int fd, bool enable_et);
    //给participant i发送信息 s
    int send_message(int i, string s);
    //接收消息,返回接收到的消息的数目
    int recv_message(string s, int n, int timeout);
    //接收participant的commmid_id
    int recv_commid_id(int n, int timeout);
    //对不一致的particpant进行恢复
    void recovery();
    //处理超时的participant
    void ProcessTimeout();
    string get_message(int i);
    void send_client_message(int conn_socket,string s);
    
    //下面三个函数是为了将命令包装成字符串给participant
    string get(string value);
    string set_command(string key, string value);
    string del_command();
public:
    coordinator(char *filename);
    //开启服务
    void start();
    ~coordinator();
};



#endif