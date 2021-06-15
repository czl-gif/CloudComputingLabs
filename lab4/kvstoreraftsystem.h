#ifndef KVSTORERAFTSYSTEM_H_INCLUDE
#define KVSTORERAFTSYSTEM_H_INCLUDE
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <sys/epoll.h>
#include <map>
#include <set>
using namespace std;
struct Info{
    string ip;
    int port;
};
struct Command
{
    string Command_name, value1, value2;
    vector<string> values;
    Command(){
        Command_name = "";
        value1 = "";
        value2 = "";
    }
};
class kvstoreraftsystem
{
private:
    int mode;//mode = 0是参与者，1是正处于投票阶段，2是leader
    int others_number;//其他成员的数量
    int epollfd;//epoll句柄
    int self_socket;//自身绑定的监听socket
    int leaderID;//leader在OtherInfo中的下标
    int commit_ID;
    Command command;
    map<string, string> memory_old;
    map<string, string> memory_new;
    Info selfInfo;//自己的IP和port,在配置文件中用coordinator表明是自己的信息
    vector<Info> OtherInfo;//其他所有成员的IP和port，包括leader
    set<int> performers;
    map<int, int>pos_connect_socket;
    map<int, int>connect_socket_pos;

    void init();
    void addfd(int epfd, int fd, bool enable_et);
    int connect_(int i);
    //构造内存语句，发送给coordinator
    string get_memory_string();
    int send_message(int i, string s);
    string read_msg(int socket);
    int write_msg(int socket, string s);
    int recv_election_message(int epfd, string s, int n, int timeout);
    int leader_recovery(int n, int timeout);
    void recoverty(char* msg);
    void timeout_process();
    void parse_command(char* msg);
    int execute_command(string s, int fd);
    int update_memory();
    void rollback();

    void election_process(int clientfd, struct sockaddr_in& serv_addr);
    void elect();
    void execute(int num, struct epoll_event * events);
public:
    kvstoreraftsystem(int argc, char *argv[]);
    void start();
    ~kvstoreraftsystem();
};



#endif