#include "kvstoreraftsystem.h"
#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
using namespace std;

kvstoreraftsystem::kvstoreraftsystem(int argc, char *argv[])
{
    clientInfo.port = 8888;
    leaderID = -1;
    commit_ID = 0;
    char *string_ = new char[2];
    char* filename = new char[255];
    mode = 0;
    static struct option long_options[]{
        {"config_path", required_argument, NULL, 1},
    };
    int opt;
    int option_index = 0;
    if((opt = getopt_long(argc, argv, string_, long_options, &option_index)) != -1){
        cout << "optarg = " << optarg << endl;
        strcpy(filename, optarg);
    }else {
        cout << "input error! please input correct config_path" << endl;
    }
    delete string_;
    struct stat s;
    if (stat(filename ,&s)==0 && s.st_mode & S_IFREG) {
        fstream input(filename);
        string str;
        while(!input.eof())
        {
            getline(input, str);
            int i = 0, n = str.length();
            while(i < n && str[i] ==' ') i++;
            if(str[i] == '!') continue;
            string parameterName, value;
            stringstream ss(str);
            if(ss.rdbuf() -> in_avail() != 0){
                ss>>parameterName;
                
                if(parameterName == "coordinator_info"){
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
                    sss >> selfInfo.ip >> selfInfo.port;
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
                    sss >> info.ip >> info.port;
                    OtherInfo.push_back(info);
                }    
                else {
                    perror("Configuration file format error");
                }
            }
        }
    }
    else {
        cout << "input error! please input correct config_path !!!" << endl;
    }
    others_number = OtherInfo.size();
    delete filename;
    init();
}

void kvstoreraftsystem::init()
{
    int backlog = 100;
    epollfd = epoll_create(backlog);
    self_socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(selfInfo.ip.c_str());
    serv_addr.sin_port = htons(selfInfo.port);
    int flag = 1;
    if (setsockopt(self_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
        perror("set coordinator_socket error!");
        exit(1);
    }
    if(bind(self_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("coordinator_socket bind error!");
        exit(1);
    }
    if(listen(self_socket, backlog) == -1) {
        perror("coordinator_socket listen error!");
        exit(1);
    }
    addfd(epollfd, self_socket, true);
}
void  kvstoreraftsystem::addfd(int epfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;//表示对应的文件描述符可以读（包括对端SOCKET正常关闭）
    if (enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    //printf("fd added to epoll!\n\n");
}

int kvstoreraftsystem::connect_(int i)
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
    seraddr.sin_port = htons(OtherInfo[i].port);
    seraddr.sin_addr.s_addr = inet_addr(OtherInfo[i].ip.c_str());
    if (connect(clientfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) != 0) 
    {
        cout << "connect" << clientfd << "error1 !" << endl;
        close(clientfd);
        clientfd = -1;
    }
    pos_connect_socket[i] = clientfd;
    return clientfd;
} 
int kvstoreraftsystem::connectclient()
{
    cout << "clientinfo:" << clientInfo.ip <<':' << clientInfo.port << endl;
    struct sockaddr_in seraddr;
    int clientfd;
    if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create participant socket error!!!");
        exit(1);
    }
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(clientInfo.port);
    seraddr.sin_addr.s_addr = inet_addr(clientInfo.ip.c_str());
    if (connect(clientfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) != 0) 
    {
        cout << "connect client" << clientfd << "error1 !" << endl;
        close(clientfd);
        clientfd = -1;
    }
    return clientfd;
}
string kvstoreraftsystem::get_memory_string()
{
    string res = "#" + to_string(commit_ID) + "\r\n";
    res += (to_string(memory_new.size()) + "\r\n");
    for(map<string, string>::iterator it = memory_new.begin(); it != memory_new.end(); it++) {
        res +=(it->first + "\r\n" + it->second + "\r\n");
    }
    return res;
}
int kvstoreraftsystem::send_message(int i, string s)
{
    int len = write(pos_connect_socket[i], s.c_str(), s.length());
    cout << "send messages to participant"<< i << ": " << s << endl;
    if(len <= 0) {
        close(pos_connect_socket[i]);
        connect_socket_pos.erase(pos_connect_socket[i]);
        pos_connect_socket[i] = -1; 
    }
    return len;
}   
string kvstoreraftsystem::read_msg(int socket)
{
    struct timeval tv_out;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 1000;
    //setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    char *buf = new char[255];
    memset(buf, 0, 255);
    int len = read(socket, buf, 255);
    if(len <= 0) {
        close(socket);
    }
    cout << "read message:" << buf << endl;
    string s;
    if(len > 0)
     s = buf;
    else s = "";
    delete buf;
    return s;
}
int kvstoreraftsystem::write_msg(int socket, string s)
{
    int len = write(socket, s.c_str(), s.length());
    if(len <= 0) {
        close(socket);
    }
    cout << "write message:" << s << endl;
    return len;
}
int kvstoreraftsystem::recv_election_message(int epfd, string s, int n, int timeout)
{
    struct epoll_event * events = new epoll_event[n];
    struct epoll_event ev;
    int num = 0;
    int res = 0;
    int p = 0;
    int t = 2;
    char *buf = new char[s.length() + 5];
    while(t--) {
        num = epoll_wait(epfd, events, n, 10);
        cout << "recvmessagenum = " << num << endl;
        for (int i = 0; i < num; i++) {
            if(events[i].data.fd == self_socket)
                continue;
            memset(buf, 0, s.length() + 5);
            if (read(events[i].data.fd, buf, s.length() + 5) > 0) {
                cout << "receve message from participant" << connect_socket_pos[events[i].data.fd]
                <<":" << buf << endl;
                if(strcmp(buf, s.c_str()) == 0) {
                    res++;
                    performers.insert(connect_socket_pos[events[i].data.fd]);
                } else if(strcmp(buf, "leader") == 0) {
                    leaderID = connect_socket_pos[events[i].data.fd];
                    p = -1;
                }
                ev.data.fd = events[i].data.fd;
                ev.events=EPOLLIN | EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
            } else {
                close(events[i].data.fd);
                pos_connect_socket[connect_socket_pos[events[i].data.fd]] = -1;
                connect_socket_pos.erase(events[i].data.fd);
            }
        }
        if(res >= n){
            t = 0;
        }
        if(t == 1)
            usleep(timeout);
    }
    delete events;
    delete buf;
    if(p == -1)
        return -1;
    return res; 
}
int kvstoreraftsystem::leader_recovery(int n, int timeout)
{
    struct epoll_event * events = new epoll_event[n];
    struct epoll_event ev;
    int num = 0;
    int res = 0;
    int p = 0;
    int t = 2;
    char *buf = new char[2550];
    map<int, int>count;
    count[commit_ID] = 1;
    while(t--) {
        num = epoll_wait(epollfd, events, n, 100);
        cout << "leader_recoverynum = " << num << endl;
        for (int i = 0; i < num; i++) {
            if(events[i].data.fd == self_socket)
                continue;
            memset(buf, 0, 2550);
            if (read(events[i].data.fd, buf, 2550) > 0) {
                cout << "收到内存信息："<< buf << endl;
                if(buf[0] != '#') continue;
                res++;
                performers.insert(connect_socket_pos[events[i].data.fd]);
                stringstream ss(buf + 1);
                int CID;
                ss >> CID;
                count[CID]++;
                if(count[CID] * 2 > n && CID != commit_ID) {
                    recoverty(buf + 1);
                }
                ev.data.fd = events[i].data.fd;
                ev.events=EPOLLIN | EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
            } else {
                close(events[i].data.fd);
                pos_connect_socket[connect_socket_pos[events[i].data.fd]] = -1;
                connect_socket_pos.erase(events[i].data.fd);
            }
        }
        if(res >= n){
            t = 0;
        }
        if(t == 1)
            usleep(timeout);
    }
    delete events;
    delete buf;
    return res; 
}
void kvstoreraftsystem::recoverty(char* msg)
{
    cout << "收到内存信息:" << endl;
    cout << msg << endl;
    stringstream ss(msg);
    int CID;
    ss >> CID;
    if(CID <= commit_ID)
        return;
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
void kvstoreraftsystem::timeout_process()
{
    for(int i = 0; i < others_number; i++) {
        if(pos_connect_socket[i] != -1 && performers.find(i) == performers.end()) {
            close(pos_connect_socket[i]);
            connect_socket_pos.erase(pos_connect_socket[i]);
            pos_connect_socket[i] = -1;
        }
    }
}

void kvstoreraftsystem::parse_command(char* msg)
{
    command.Command_name = "";
    command.value1 = command.value2 = "";
    command.values.clear();
    stringstream ss(msg);
    int n;ss >> n;
    string m,s;
    ss >> m >> command.Command_name;
    if(command.Command_name == "SET") {
        ss >> m >> command.value1;
        command.value2 = "";
        ss >> m >> command.value2;
        for(int k = 3; k < n; k++) {
            ss >> m >> s;
            command.value2 += (" " + s);
        }
    }
    else if(command.Command_name == "GET") {
        ss >> m >> command.value1;
    }
    else {
        for(int k = 1; k < n; k++) {
            ss >> m >> s;
            command.values.push_back(s);
        }
    }
    cout << "command name:" << command.Command_name <<' ' << command.value1<<' '<<command.value2<< endl;
    for(int k = 0; k < command.values.size(); k++)
        cout << command.values[k] << endl;
    
}
int kvstoreraftsystem::execute_command(string s, int fd)
{
    cout << "execute command:" << command.Command_name << endl;
    if(command.Command_name == "GET") {
        int k = 0;
        for(int i = 0; i < others_number; i++) {
            if(pos_connect_socket[i] != -1) {
                k++;
                send_message(i, "memory");
            }
        }
        cout << "执行命令k=" << k << endl;
        int res = leader_recovery(k, 1000000);
        if(res < k)
            timeout_process();
        cout << "执行命令res = "<< res << endl;
        if((res +  1) * 2 > others_number) {
            string str;
            if(memory_new.find(command.value1) != memory_new.end()) {
                str = memory_new[command.value1];
            }else {
                str = "nil";
            }
            int n = 1;
            for(int i = 0; i < str.length(); i++)
                if(str[i] == ' ') n++;
            string buf = "*" + (to_string(n) + "\r\n");
            stringstream ss(str);string s0;
            while(n--) {
                ss >> s0;
                buf += ("$" + to_string(s0.length()) + "\r\n" + s0 + "\r\n");
            }
            write_msg(fd, buf);
        }
        return res;
    }
    else {
        int k = 0;
        if(command.Command_name == "DEL") {
             for(int i = 0; i < others_number; i++) {
                if(pos_connect_socket[i] != -1) {
                    k++;
                    send_message(i, "memory");
                }
            }
            int res = leader_recovery(k, 1000000);
            cout << "delres = " << res << endl;
            if(res < k)
                timeout_process();
            if(res * 2 < others_number) {
                return res;
            }
        }
        k = 0;
        for(int i = 0; i < others_number; i++) {
            if(pos_connect_socket[i] != -1) {
                k++;
                send_message(i, s);
            }
        }
        cout << "执行命令k=" << k << endl;
        int res = recv_election_message(epollfd, "ACK", k, 3000000);
        if(res < k)
            timeout_process();
        cout << "执行命令res = "<< res << endl;
        if(command.Command_name == "SET") {
            
            if((res +  1) * 2 > others_number) {
                update_memory();
                write_msg(fd, "+OK\r\n");
            }else {
                for(int i = 0; i < others_number; i++) {
                    if(pos_connect_socket[i] != -1) {
                        k++;
                        send_message(i, "rollback");
                    }
                }
            }
            return res;
        }
        else if(command.Command_name == "DEL") {
            if((res +  1) * 2 > others_number) {
                int len = update_memory();
                write_msg(fd, ":" + to_string(len) + "\r\n");
            }else {
                for(int i = 0; i < others_number; i++) {
                    if(pos_connect_socket[i] != -1) {
                        k++;
                        send_message(i, "rollback");
                    }
                }
            }
            return res;
        }        
    }
    return 0; 
}
int kvstoreraftsystem::update_memory()
{
    if(command.Command_name == "GET")
        return 0;
    memory_old.clear();
    for(map<string, string>::iterator it = memory_new.begin(); it != memory_new.end(); it++){
        memory_old[it->first] = it->second;
    }
    commit_ID++;
    if(command.Command_name == "SET") {
        memory_new[command.value1] = command.value2;
        cout << "更新内存信息：" << get_memory_string() << endl;
        return 0;
    }else if(command.Command_name == "DEL") {
        int res = 0;
        for(int i = 0; i < command.values.size(); i++) {
            if(memory_new.find(command.values[i]) != memory_new.end()) {
                res ++;
                memory_new.erase(command.values[i]);
            }
        }
        return res;
    }
    return 0;
}
void kvstoreraftsystem::rollback()
{
    commit_ID--;
    memory_new.clear();
    for(map<string, string>::iterator it = memory_old.begin(); it != memory_old.end(); it++) {
        memory_new.insert(make_pair(it->first, it->second));
    }
}
void kvstoreraftsystem::election_process(int clientfd, struct sockaddr_in& serv_addr)
{
    if(mode == 0) {
        write_msg(clientfd, "vote");
        mode = 1;
        string s = read_msg(clientfd);
        if(s == "") {
            //close(clientfd);
            mode = 0;
            return;
        }else{
            cout << "new leader!!!" << endl;
            int j;
            for(j = 0; j < others_number; j++) {
                if(s == to_string(OtherInfo[j].port)) break;
            }
            if(j >= others_number) {
                close(clientfd);
                return;
            }
            if(leaderID != -1 && pos_connect_socket[leaderID] != -1) {
                close(pos_connect_socket[leaderID]);
                connect_socket_pos.erase(pos_connect_socket[leaderID]);
                pos_connect_socket[leaderID] = -1;
            } 
            leaderID = j;
            pos_connect_socket[leaderID] = clientfd;
            connect_socket_pos[clientfd] = leaderID;
            mode = 0;
            addfd(epollfd, clientfd, true); 
            string s = get_memory_string();
            write_msg(clientfd, s);       
        }
    }
    else if(mode == 2) {
        write_msg(clientfd, "leader");
        int j;
        for(j = 0; j < others_number; j++) {
            if(serv_addr.sin_addr.s_addr != inet_addr(OtherInfo[j].ip.c_str()))
                break;
        }
        close(clientfd);
        if(j >= others_number)
            return;
        clientfd = connect_(j);
        addfd(epollfd, clientfd, true);
        pos_connect_socket[j] = clientfd;
        connect_socket_pos[clientfd] = j;
        int n = 0;
        for(int j = 0; j < others_number; j++) {
            if(pos_connect_socket[j] != -1) {
                n++;
                send_message(j, "memory");
            }
        }
        performers.clear();
        if(leader_recovery(n, 500000) < n)
            timeout_process();

    }
}
void kvstoreraftsystem::elect()
{
    if(mode == 2) {
        for(int i = 0; i < others_number; i++) {
                if(pos_connect_socket[i] != -1) {
                    close(pos_connect_socket[i]);
                    connect_socket_pos.erase(pos_connect_socket[i]);
                    pos_connect_socket[i] = -1;
                }
            }
    }
    mode = 1;
    if(leaderID != -1)
        close(pos_connect_socket[leaderID]);
    leaderID = -1;
    int n = 0;
    for(int i = 0; i < others_number;i++) {
        int fd = connect_(i);
        if(fd != -1) {
            connect_socket_pos[fd] = i;
            if(send_message(i, "election") > 0) {
                n++;
                addfd(epollfd, fd, true);
            }   
        }
    }
    if((n + 1) * 2 > others_number) {
        performers.clear();
        int m = n;
        n = recv_election_message(epollfd, "vote", n, 500000);
        if (n == -1) {
            mode = 0;
            for(int i = 0; i < others_number; i++) {
                if(pos_connect_socket[i] != -1) {
                    close(pos_connect_socket[i]);
                    connect_socket_pos.erase(pos_connect_socket[i]);
                    pos_connect_socket[i] = -1;
                }
            }
        }
        else if((n + 1) * 2 > others_number) {
            if(m < n) timeout_process();
            mode = 2;
            int k = 0;
            for(int i = 0; i < others_number; i++) {
                if(pos_connect_socket[i] != -1) {
                    if(performers.find(i) != performers.end()) {
                        send_message(i, to_string(selfInfo.port));
                        k++;
                    }
                    else {
                        close(pos_connect_socket[i]);
                        connect_socket_pos.erase(pos_connect_socket[i]);
                        pos_connect_socket[i] = -1;
                    }
                }
            }
            performers.clear();
            if(leader_recovery(k,500000) < k)
                timeout_process();
            //恢复
        }else {
            mode = 0;
        }
    }
    else {
        mode = 0;
        for(int i = 0; i < n; i ++) {
            if(pos_connect_socket[i] != -1) {
                close(pos_connect_socket[i]);
                connect_socket_pos.erase(pos_connect_socket[i]);
                pos_connect_socket[i] = -1;  
            }
        }            
    }
}
void kvstoreraftsystem::execute(int num, struct epoll_event * events)
{
    struct sockaddr_in serv_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    for(int i = 0; i < num; i++) {
        if(events[i].data.fd == self_socket) {
            memset(&serv_addr, 0, sizeof(serv_addr));
            int clientfd = accept(self_socket, (struct sockaddr*)&serv_addr, &sin_size);
            getpeername(clientfd, (struct sockaddr*)&serv_addr, &sin_size);
            string s = read_msg(clientfd);
            if(s == "") continue;
            else if(s == "election") {
                
                election_process(clientfd, serv_addr);
            }
            else if(s[0] == '*' || s[0] == '&') {
                if(mode == 0) {
                    if(leaderID != -1) {
                        int clientfd = connect_(leaderID);
                        string s0 = inet_ntoa(serv_addr.sin_addr);
                        if(clientfd != -1) {
                            write_msg(clientfd,"&" + s0 + "\r\n" + s);
                            close(clientfd);
                            continue;
                        }
                    }
                    elect(); 
                }
                if(mode == 2) {
                    //处理命令
                    int p = 1;
                    if(s[0] == '&') {
                        clientInfo.ip = "";
                        for(p = 1; p < s.length(); p++){
                            if(s[p] == '\r') break;
                            clientInfo.ip += s[p];
                        }
                        p += 3;
                    }else {
                        clientInfo.ip = inet_ntoa(serv_addr.sin_addr);
                        p = 1;
                    }

                    close(clientfd);
                    char* msg = new char[s.length()];
                    memset(msg,0,s.length());
                    for(int i = p; i < s.length(); i++)
                        msg[i - p] = s[i];
                    msg[s.length()] = '\0';
                    cout << "p = " << p << "msg = " << msg << endl;
                    parse_command(msg);
                    s = msg;
                    delete msg;
                    clientfd = connectclient();
                    
                    int res = execute_command( "*" + s, clientfd);
                    if(res * 2 < others_number)
                        write_msg(clientfd, "-ERROR\r\n");
                    close(clientfd);
                }
            }
        }else {
            string s = read_msg(events[i].data.fd);
            if(s == "memory") {
                s = get_memory_string();
                write_msg(events[i].data.fd, s);
            }
            else if(s[0] == '#') {
                char* msg = new char[s.length()];
                for(int i = 1; i < s.length(); i++)
                    msg[i - 1] = s[i];
                msg[s.length()] = '\0';
                recoverty(msg);
                delete msg;
            }
            else if(s[0] == '*') {
                char* msg = new char[s.length()];
                for(int i = 1; i < s.length(); i++)
                    msg[i - 1] = s[i];
                msg[s.length()] = '\0';
                parse_command(msg);
                delete msg;
                update_memory();
                write_msg(events[i].data.fd, "ACK");
            }
            else if(s == "rollback"){
                rollback();
            }
            else if(s == "") {//有断开时close的
                cout << "有socket close"<< endl;
                if(mode == 0) {
                    cout << "leaderID = " << leaderID << "leader socket = "<< pos_connect_socket[leaderID]
                    << "events[i].data.fd = "<< events[i].data.fd << endl;
                    if(leaderID != -1 && pos_connect_socket[leaderID] == events[i].data.fd){
                        pos_connect_socket[leaderID] = -1;
                        connect_socket_pos.erase(events[i].data.fd);
                        leaderID = -1;
                    }
                    close(events[i].data.fd);
                }else if(mode == 2) {
                    close(events[i].data.fd);
                    pos_connect_socket[connect_socket_pos[events[i].data.fd]] = -1;
                    connect_socket_pos.erase(events[i].data.fd);
                }
            }
        }
    }
}
void kvstoreraftsystem::start()
{
    struct epoll_event * events = new epoll_event[others_number + 5];
    struct epoll_event ev;
    int time_ = 0;
    struct sockaddr_in seraddr;
    for(int i = 0; i < others_number; i++) {
        cout << OtherInfo[i].ip << endl;
    }
    cout << "othernumber=" << others_number<< endl;
    cout << selfInfo.ip << endl;
    while(1) {
        time_ = 0;
        while(time_ < 200) {
            srand((int)time(0));
            time_ = rand() % 1600;
        }
        int time0 = 0;
        while(time0 <= 300){
            srand((int)time(0));
            time0 = rand() % 1000;
        }
        time_ *= time0;
        if(commit_ID == 0) time_ /= 10;
        if(mode == 2) {
            time_ *= 10;
            cout << "I'm leader:" << selfInfo.ip<< ':'<< selfInfo.port << endl;
        }
        cout << "time_ = " << time_ << endl;
        cout << "leaderID = " << leaderID << endl; 
        cout << "mode =" << mode << endl;
        memset(events, 0, sizeof(struct epoll_event) * (others_number + 5)); 
        int num = epoll_wait(epollfd, events, others_number + 5, time_);
        cout << "mainnum =" << num << endl;
        if(num <= 0) {
            elect(); 
        }
        else {
            execute(num, events);
        }
    }
}
kvstoreraftsystem::~kvstoreraftsystem()
{
}