#ifndef PARTICIPANT_H_INCLUDE
#define PARTICIPANT_H_INCLUDE
#include <iostream>
#include "kvstore2pcsystem.h"
#include <string>
#include <map>
#include <sys/epoll.h>
using namespace std;

class participant
{
private:
    map<string, string> memory_old;
    map<string, string> memory_new;
    Command command;
    Info coordinator_info;
    Info participant_info;
    int participant_socket;
    
    int commit_ID;
    int backlog;
    int del_number;
    void back_up();
    string execute_command();
    void rollback();
    int send_message(int conn_socket, string s);
    void process(int conn_socket);
    //
    void connect_coordinator();
    void init();
    //构造内存语句，发送给coordinator
    string get_memory_string();
    void recovery(char * msg);
    void parse_message(char *msg);
    //下面三个函数是为了解析命令套用的
    int get_nextrn(char* msg, int l);
    int get_number(char* msg, int l, int r);
    string get_string(char* msg, int l, int r);

public:
    //构造函数，从配置文件中读取内容并初始化相应变量
    participant(char * filename);
    void start();
    ~participant();
};


#endif