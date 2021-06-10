#include "participant.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace std;

void participant::init()
{
    cout << "coordinator information = " << coordinator_info.ip <<':' << coordinator_info.port << endl;
    cout << "participant information = " << participant_info.ip << ':' << participant_info.port << endl;
    participant_socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(participant_info.ip.c_str());
    serv_addr.sin_port = htons(participant_info.port);
    int flag = 1;
    if (setsockopt(participant_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
        perror("set coordinator_socket error!");
        exit(1);
    }
    if(bind(participant_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("participant_socket bind error!");
        exit(1);
    }
    if(listen(participant_socket, backlog) == -1) {
        perror("participant_socket listen error!");
        exit(1);
    }
    connect_coordinator();
}
void participant::connect_coordinator()
{
    struct sockaddr_in seraddr;
    int clientfd;
    if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create participant socket error!!!");
        exit(1);
    }
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(coordinator_info.port);
    seraddr.sin_addr.s_addr = inet_addr(coordinator_info.ip.c_str());
    while(connect(clientfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) != 0) {
        cout << "connect coordinator error" << endl;
    }
    close(clientfd);
}
//构造函数，从配置文件中读取内容并初始化相应变量
participant::participant(char * filename)
{
    backlog = 20;
    fstream input(filename);
    string str;
    commit_ID = 0;
    while(!input.eof()) 
    {
        getline(input, str);
        int i = 0, n = str.length();
        while(i < n && str[i] ==' ') i++;
        if(str[i] == '!') continue;
        string parameterName, value;
        stringstream ss(str);
        if(ss.rdbuf() -> in_avail() != 0) {
            ss >> parameterName;
            if (parameterName == "mode") continue;
            else if(parameterName == "coordinator_info") {
                if(ss.rdbuf() -> in_avail() == 0) {
                    perror("coordinator_info is null in the configuration file！");
                    exit(0);
                }
                ss >> value;
                int j = 0;
                for(j = 0; j < value.length(); j++)
                    if(value[j] == ':') {
                        value[j] = ' ';break;
                    }
                if(j >= value.length()) {
                    perror("there isn't a port in the coordinator_info !");
                    exit(0);
                }
                stringstream sss(value);
                sss >> coordinator_info.ip >> coordinator_info.port;
            }
            else if(parameterName == "participant_info"){
                if(ss.rdbuf() -> in_avail() == 0) {
                    perror("participant_info is null in the configuration file！");
                    exit(0);
                }
                ss >> value;
                int j = 0;
                for(j = 0; j < value.length(); j++)
                    if(value[j] == ':') {
                        value[j] = ' ';break;
                    }
                if(j >= value.length()) {
                    perror("there isn't a port in the participant_info !");
                    exit(0);
                }
                stringstream sss(value);
                Info info;
                sss >> participant_info.ip >> participant_info.port;
            }
        }
    }
    init();
}

void participant::start()
{
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    socklen_t sin_size = sizeof(struct sockaddr_in);
    socklen_t nl = sizeof(serv_addr);
    while(1)
    {
        int conn_sock = accept(participant_socket, (struct sockaddr*)&serv_addr, &sin_size);
        if (conn_sock < 0) {
            perror("connect error!");
            exit(1);
        }    
        if(serv_addr.sin_addr.s_addr != inet_addr(coordinator_info.ip.c_str()))
        {
            close(conn_sock);
            continue;
        }
        process(conn_sock);
        
    }
}
void participant::process(int conn_socket)
{
    char *s = new char[255];
    while(1) {
        memset(s, 0, 255);
        if(read(conn_socket, s, 255) > 0){
            
            string buf = s;
            cout << "receve message from coordiantor:" << buf << endl;    
            if(buf == "Hello!") {
                cout << "收到心跳包" << endl;
                send_message(conn_socket, to_string(commit_ID));
            }else if(s[0] == '*') {
                parse_message(s);
                if(command.Command_name == "GET") {
                    send_message(conn_socket, execute_command());
                    continue;
                }
                send_message(conn_socket, "prepared");
                memset(s, 0, 255);
                if(read(conn_socket, s, 255) > 0) {
                    string msg = s;
                    if(msg == "DO") 
                        execute_command();
                    send_message(conn_socket, "ACK");
                }else {
                    close(conn_socket);
                    break;
                }
            }
            else if(buf == "del number") {
                cout << "请求成功删除个数" << endl;
                send_message(conn_socket, to_string(del_number));
            }
            else if(buf == "rollback") {
                rollback();
                send_message(conn_socket, "rollback_successed");
            }
            else if(buf == "&") {
                string s = get_memory_string();
                send_message(conn_socket, s);
            }
            else if(s[0] == '#') {
                s[0] = ' ';
                recovery(s + 1);
                send_message(conn_socket, "OK");
            }
        }else {
            close(conn_socket);
            break;
        }
        cout << "内存信息如下："<<commit_ID << endl;
        for(map<string, string>::iterator it = memory_new.begin(); it != memory_new.end(); it++) {
            cout<< conn_socket << it ->first <<' ' << it->second << endl;
        }
    }
    delete s;
}

string participant::execute_command()
{
    back_up();
    //cout << "执行命令"<<command. << command.value1 << "zhi:" << memory_new[command.value1] << endl;
    if(command.Command_name == "GET") {
        if(memory_new.find(command.value1) != memory_new.end()) {
            return memory_new[command.value1];
        }else {
            cout << "请求键值=" << command.value1 << endl;
            cout << "此时内存::"<< endl;
            for(map<string, string>::iterator it = memory_new.begin(); it != memory_new.end(); it++) {
                cout << it ->first <<' ' << it->second << endl;
            }
            return "nil";
        }
    }
    else if(command.Command_name == "SET") {
        memory_new[command.value1] = command.value2;
    }
    else if(command.Command_name == "DEL") {
        int k = 0;
        for(int i = 0; i < command.values.size(); i++) {
            if(memory_new.find(command.values[i]) != memory_new.end()) {
                k++;
                memory_new.erase(command.values[i]);
            }
        }
        del_number = k;
    }
    commit_ID ++;
    return "";
}
void participant::rollback()
{
    commit_ID--;
    memory_new.clear();
    for(map<string, string>::iterator it = memory_old.begin(); it != memory_old.end(); it++) {
        memory_new.insert(make_pair(it->first, it->second));
    }
}
void participant::back_up()
{
    memory_old.clear();
    for(map<string, string>::iterator it = memory_new.begin(); it != memory_new.end(); it++) {
        memory_old.insert(make_pair(it->first, it->second));
    }
}
void participant::parse_message(char *msg)
{
    int i = 1;int len = strlen(msg);
    int j = get_nextrn(msg, i);
    int n = get_number(msg, i, j);
    i = j + 3;
    j = get_nextrn(msg, i);
    int m = get_number(msg, i , j);
    i = j + 2;
    j = get_nextrn(msg, i);
    command.Command_name = get_string(msg, i, i + m);
    i = i + m + 3;
    if(command.Command_name == "SET") {
        j = get_nextrn(msg, i);
        m = get_number(msg, i , j);
        i = j + 2;
        command.value1 = get_string(msg, i, i + m);
        i = i + m + 3;
        j = get_nextrn(msg, i);
        //cout << "len = " << len << "j = " << j << endl;
        m = get_number(msg, i , j);
        i = j + 2;
        command.value2 = get_string(msg, i, i + m);
    }
    else if(command.Command_name == "GET") {
        j = get_nextrn(msg, i);
        m = get_number(msg, i , j);
        i = j + 2;
        command.value1 = get_string(msg, i, i + m);
    }
    else {
        for(int k = 1; k < n; k++) {
            j = get_nextrn(msg, i);
            m = get_number(msg, i , j);
            i = j + 2;
            command.values.push_back(get_string(msg, i, i + m));
            i = i + m + 3;
        }
    }
    
}
int participant::send_message(int conn_socket, string s)
{
    int len = write(conn_socket, s.c_str(), s.length());
    cout << "send message to coordinator:" << s.c_str() << endl;
    return len;
}


int participant::get_nextrn(char* msg, int l)
{
    for(int i = l;;i++) {
        if(msg[i] == '\r' && msg[i + 1] == '\n')
            return i;
    }
}
int participant::get_number(char* msg, int l, int r)
{
    int res = 0;
    for(int i = l; i < r; i++) {
        res *= 10;
        res += (msg[i] - '0');
    }
    return res;
}
string participant::get_string(char * msg, int l, int r)
{
    string res;
    for(int i = l; i < r; i++) {
        res += msg[i];
    }
    return res;
}
string participant::get_memory_string()
{
    string res = "#" + to_string(commit_ID) + "\r\n";
    res += (to_string(memory_new.size()) + "\r\n");
    for(map<string, string>::iterator it = memory_new.begin(); it != memory_new.end(); it++) {
        res +=(it->first + "\r\n" + it->second + "\r\n");
    }
    return res;
}
void participant::recovery(char *msg)
{
    cout << "收到内存信息:" << endl;
    cout << msg << endl;
    stringstream ss(msg);
    ss >> commit_ID;
    int n;
    ss >> n;
    string s1, s2;
    string s0;
    memory_new.clear();
    for(int i = 0; i < n; i++) {
        ss >> s1;
        getline(ss, s0);
        getline(ss, s2);
        cout<<"键值=" << s1 << ' ' << s2;
        memory_new[s1] = s2;
    }
}
participant::~participant()
{

}
