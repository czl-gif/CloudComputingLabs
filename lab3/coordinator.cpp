#include "coordinator.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <queue>
using namespace std;

//将socket描述
void  coordinator::addfd(int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;//表示对应的文件描述符可以读（包括对端SOCKET正常关闭）
    if (enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    //printf("fd added to epoll!\n\n");
}

coordinator::coordinator(char *filename)
{
    fstream input(filename);
    string str;
    backlog = 100;
    epollfd = epoll_create(backlog);
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
                sss >> info.ip >> info.port;
                participant_info.push_back(info);
            }
        }
    }
    participant_number = participant_info.size();
    for(int i = 0; i < participant_number; i++) {
        failed_participant.insert(i);
        
        cout << participant_info[i].ip << ':' << participant_info[i].port << endl;
    }
    init();
}
void coordinator::participant_work_init()
{
    participant_work.clear();
    for(int i = 0; i < participant_number; i++)
        participant_work.push_back(false);
}
void coordinator::commid_ID_init()
{
    participants_commit_ID.clear();
    for(int i = 0; i < participant_number; i++)
        participants_commit_ID.push_back(-1);
}
void coordinator::init()
{
    coordinator_socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(coordinator_info.ip.c_str());
    serv_addr.sin_port = htons(coordinator_info.port);
    int flag = 1;
    if (setsockopt(coordinator_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
        perror("set coordinator_socket error!");
        exit(1);
    }
    if(bind(coordinator_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("coordinator_socket bind error!");
        exit(1);
    }
    if(listen(coordinator_socket, backlog) == -1) {
        perror("coordinator_socket listen error!");
        exit(1);
    }
    Heartbeat_detection(5);
}

void coordinator::Heartbeat_detection(int mode)
{
    set<int>::iterator it;
    struct sockaddr_in seraddr;
    if (mode == 3 || mode == 5) {
        vector<thread> t;
        for(it = failed_participant.begin();it != failed_participant.end();) {
            int i = *it;
           
            int clientfd;
            if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("create participant socket error!!!");
                exit(1);
            }
            memset(&seraddr, 0, sizeof(seraddr));
            seraddr.sin_family = AF_INET;
            seraddr.sin_port = htons(participant_info[i].port);
            seraddr.sin_addr.s_addr = inet_addr(participant_info[i].ip.c_str());
            if ((connect(clientfd, (struct sockaddr*)&seraddr, sizeof(seraddr))) != 0)
            {
                cout << "connect" << clientfd << "error1 !" << endl;
                if(mode == 5)
                    t.push_back(move(thread(try_connect,this, i, clientfd, seraddr)));
                else 
                    close(clientfd);
                it ++;
                continue;
            };
            it++;
            failed_participant.erase(i);
            succeed_participant.insert(i);
            connect_participant_pos_socket[i] = clientfd;
            connect_participant_socket_pos[clientfd] = i;
            addfd(clientfd, true);
        }
        if (mode == 5) {
            for(int i = 0; i < t.size(); i++) 
            t[i].join();
        }
    }
    if (mode == 2 || mode == 3 || mode == 5) {
        for(it = succeed_participant.begin();it != succeed_participant.end();) {
            int i = *it;
            if(send_message(i, "Hello!") <= 0) {
                cout << "participant i send Hello error" << endl;
                it ++;
                succeed_participant.erase(i);
                failed_participant.insert(i);
                close(connect_participant_pos_socket[i]);
                continue;
            }
            it++;
        }
        participant_work_init();
        commid_ID_init();
        cout << "succeed size1 = " << succeed_participant.size() << endl;
        int n = succeed_participant.size();
        if(recv_commid_id(n, 500000) != n) {
            ProcessTimeout();
        }

    }
    cout << "succeed size2 = " << succeed_participant.size() << endl;
}

void coordinator::try_connect(coordinator* cor,int i, int clientfd, struct sockaddr_in seraddr)
{
    int n = 20;
    while (n--)
    {
        if ((connect(clientfd, (struct sockaddr*)&seraddr, sizeof(seraddr))) == 0)
        {
            cor->failed_participant.erase(i);
            cor->succeed_participant.insert(i);
            cor->connect_participant_pos_socket[i] = clientfd;
            cor->connect_participant_socket_pos[clientfd] = i;
            cor->addfd(clientfd, true);
            return;                
        };
        usleep(50000);
    }
    close(clientfd);
    
    string s = "connect participant";
    s+= to_string(i); 
    s+= "error";
    cout << s << endl;
}
int coordinator::send_message(int i, string s)
{
    int len = write(connect_participant_pos_socket[i], s.c_str(), s.length());
    cout << "send messages to participant: " << s << endl;
    return len;
}


void coordinator::ProcessTimeout()
{
    for(int i = 0; i < participant_number; i++){
        if(participant_work[i] == false && succeed_participant.find(i) != succeed_participant.end()){
            succeed_participant.erase(i);
            failed_participant.insert(i);
            close(connect_participant_pos_socket[i]);
        }
    }
}
/*void coordinator::parse(char* msg)
{
    stringstream ss(msg);
    int n; string m; ss >> n;
    ss >> m >> command.Command_name;
    if(command.Command_name == "SET") {
        ss >> m >> command.value1;
        ss >> m >> command.value2;
    }
    else if(command.Command_name == "GET") {
        ss >> m >> command.value1;
    }
    else {
        for(int i = 1; i < n; i++) {
            ss >> m >> m;
            command.values.push_back(m);
        }
    }
}*/


int coordinator::RecvCommand(int conn_socket)
{
    char * msg = new char[65536];
    memset(msg, 0, strlen(msg));
    int len = read(conn_socket, msg, 65536 * sizeof(char));
    if (len <= 0 || msg[0] != '*') {
        return 1;
    }
    stringstream ss(msg + 1);
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
    return 0;
}

string coordinator::get(string value)
{
    string res = "*2\r\n$3\r\nGET\r\n$";
    res += to_string(value.length());
    res += "\r\n";
    res += value;
    res += "\r\n";
    return res; 
}
string coordinator::set_command(string key, string value)
{
    string res = "*3\r\n$3\r\nSET\r\n$";
    res += to_string(key.length());
    res += "\r\n";
    res += key;
    res += "\r\n";
    res += "$";
    res += to_string(value.length());
    res += "\r\n";
    res += value;
    res += "\r\n";
    return res;
}
string coordinator::del_command()
{
    string res = "*" + to_string(command.values.size() + 1) +"\r\n$3\r\nDEL\r\n";
    for(int i = 0; i < command.values.size(); i++) {
        res += ("$" + to_string(command.values[i].size()) +"\r\n" + command.values[i] + "\r\n");
    } 
    return res;
}
void coordinator::start()
{
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int id = 0;
    int order = 0;
    while(1) 
    {
        order ++;
        if(order >= 2) {
            Heartbeat_detection(3);
            order = 0;
        }
        command.value1 = command.Command_name = command.value2 = "";
        command.values.clear();
        if(id >= participant_number) id -= participant_number;
        int conn_sock = accept(coordinator_socket, (struct sockaddr*)&serv_addr, &sin_size);
        if (conn_sock < 0) {
            perror("connect error!");
            exit(1);
        }
        else {
            cout << conn_sock << "connect!!!" << endl;
        }
        

        if(RecvCommand(conn_sock) == 1) {
            Heartbeat_detection(3);
            continue;
        }
        cout << "succeed size = " << succeed_participant.size() << endl;
        //Heartbeat_detection(3);
        if(succeed_participant.size() == 0) {
            string s = "-ERROR\r\n";
            send_client_message(conn_sock, s);
            continue;
        }
        if(command.Command_name == "GET") {
            string s = get(command.value1);
            while(succeed_participant.find(id) == succeed_participant.end())
                id++;
            cout << "id=" << id << endl;
            send_message(id, s);
            
            string res;
            while((res = get_message(id)) == "") {
                if(succeed_participant.find(id) != succeed_participant.end()) {
                    succeed_participant.erase(id);
                    failed_participant.insert(id);
                    close(connect_participant_pos_socket[id]);
                }
                if(succeed_participant.size() == 0) {
                    string s = "-ERROR\r\n";
                    send_client_message(conn_sock, s);
                    break;
                }
                while(succeed_participant.find(id) == succeed_participant.end())
                    id++;
                send_message(id, s);
            } 
            if(res == "") continue;
            int n = 1;
            for(int i = 0; i < res.length(); i++)
                if(res[i] == ' ') n++;
            string str = "*";
            str += (to_string(n) + "\r\n");
            stringstream ss(res);
            while(n--) {
                ss >> s;
                str += ("$" + to_string(s.length()) + "\r\n" + s + "\r\n");
            }
            send_client_message(conn_sock, str);
            continue;
        }
        else if(command.Command_name == "SET") {
            string s = set_command(command.value1, command.value2);
            set<int>::iterator it;
            for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                int i = *it;
                send_message(i, s);
            }
            int n = succeed_participant.size();
            participant_work_init();
            if(recv_message("prepared", n, 500000) == n) {
                for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                    int i = *it;
                    send_message(i, "DO");
                }
                if(recv_message("ACK", n, 500000) < n) {
                    ProcessTimeout();
                    for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                    int i = *it;
                    send_message(i, "rollback");
                    }
                    n = succeed_participant.size();
                    if(recv_message("rollback_successed", n, 500) < n)
                        ProcessTimeout();
                } 
                else {
                    string s = "+OK\r\n";
                    send_client_message(conn_sock, s);
                    continue;
                }
            }
            else {
                ProcessTimeout();
                for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                    int i = *it;
                    send_message(i, "Abort");
                }
                n = succeed_participant.size();
                if(recv_message("ACK", n, 500000) < n) {
                    ProcessTimeout();
                }
            }
        }
        //处理del命令
        else if(command.Command_name == "DEL") {
            string s = del_command();
            set<int>::iterator it;
            for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                int i = *it;
                send_message(i, s);
            }
            int n = succeed_participant.size();
            participant_work_init();
            if(recv_message("prepared", n, 500000) == n) {
                for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                    int i = *it;
                    send_message(i, "DO");
                }
                if(recv_message("ACK", n, 500000) < n) {
                    ProcessTimeout();
                    for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                    int i = *it;
                    send_message(i, "rollback");
                    }
                    n = succeed_participant.size();
                    if(recv_message("rollback_successed", n, 500) < n)
                        ProcessTimeout();
                } 
                else {
                    while(succeed_participant.find(id) == succeed_participant.end()) 
                        id++;
                    string s = "del number";
                    send_message(id, "del number");
                    
                    string res;
                    while((res = get_message(id)) == "") {
                        if(succeed_participant.find(id) != succeed_participant.end()) {
                            succeed_participant.erase(id);
                            failed_participant.insert(id);
                            close(connect_participant_pos_socket[id]);
                        }
                        if(succeed_participant.size() == 0) {
                            string s = "-ERROR\r\n";
                            send_client_message(conn_sock, s);
                            break;
                        }
                        while(succeed_participant.find(id) == succeed_participant.end())
                            id++;
                        send_message(id, s);
                    }
                    send_client_message(conn_sock, ":" + res + "\r\n");
                    continue;
                }
            }
            else {
                ProcessTimeout();
                for(it = succeed_participant.begin(); it != succeed_participant.end(); it++) {
                    int i = *it;
                    send_message(i, "Abort");
                }
                n = succeed_participant.size();
                if(recv_message("ACK", n, 500000) < n) {
                    ProcessTimeout();
                }
            }
        }
        string s = "-ERROR\r\n";
        send_client_message(conn_sock, s);
        close(conn_sock);
    }
}

void coordinator::send_client_message(int conn_socket, string s)
{
    int len = write(conn_socket, s.c_str(), s.length());
    if(len <= 0) {
        cout << "send messages to client error!!!" << endl;
    }
    cout << "send messages to client: " << s << endl;
    return;
}
string coordinator::get_message(int i)
{
    struct epoll_event * events = new epoll_event[20];
    struct epoll_event ev;
    int num = 0;
    char *buf = new char[255];
    string res;
    int t = 2;
    while(t--) {
        num = epoll_wait(epollfd, events, 18, 10);
        cout << "get participant num" << endl;
        for(int i = 0; i < num; i++) {
            memset(buf, 0, 255);
            if(events[i].data.fd == connect_participant_pos_socket[i]) t = 0;
            int len;
            if((len = read(events[i].data.fd, buf, 255)) > 0) {
                buf[len] = '\0';
                cout << "receve message from participant:" << buf << endl;
                ev.data.fd = events[i].data.fd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
                res = buf;
            } else {
                int k = connect_participant_socket_pos[events[i].data.fd];
                succeed_participant.erase(k);
                failed_participant.insert(k);
                close(events[i].data.fd);
            }
        }
        if(t == 1)
            usleep(300000);
    }
    delete events;
    delete buf;
    return res;  
}

int coordinator::recv_message(string s, int n, int timeout)
{
    struct epoll_event * events = new epoll_event[n];
    struct epoll_event ev;
    int num = 0;
    int res = 0;
    int t = 2;
    char *buf = new char[s.length() + 5];
    while(t--) {
        num = epoll_wait(epollfd, events, n, 10);
        cout << "num = " << num << endl;
        for (int i = 0; i < num; i++) {
            memset(buf, 0, s.length() + 5);
            if (read(events[i].data.fd, buf, s.length() + 5) > 0) {
                cout << "receve message from participant:" << buf << endl;
                if(strcmp(buf, s.c_str()) == 0) {
                    res++;
                    participant_work[connect_participant_socket_pos[events[i].data.fd]] = true;
                }else {
                    participant_work[connect_participant_socket_pos[events[i].data.fd]] = true;
                }
                ev.data.fd = events[i].data.fd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
            } else {
                int k = connect_participant_socket_pos[events[i].data.fd];
                succeed_participant.erase(k);
                failed_participant.insert(k);
                close(events[i].data.fd);
            }
        }
        if(num >= n){
            delete events;
            delete buf;
            return res;
        }
        if(t == 1)
            usleep(timeout);
    }
    delete events;
    delete buf;
    return res; 
}

int coordinator::recv_commid_id(int n, int timeout)
{
    struct epoll_event * events = new epoll_event[n];
    struct epoll_event ev;
    int num = 0;
    int res = 0;
    int t = 2;
    char *buf = new char[255];
    while(t--) {
        num = epoll_wait(epollfd, events, n, 10);
        cout << "num = " << num << endl;
        for (int i = 0; i < num; i++) {
            memset(buf, 0, 255);
            if (read(events[i].data.fd, buf, 255) > 0) {
                cout << "receve message from participant:" << buf << endl;
                int k = atoi(buf);
                participants_commit_ID[connect_participant_socket_pos[events[i].data.fd]] = k;
                res++;
                participant_work[connect_participant_socket_pos[events[i].data.fd]] = true;
                
                ev.data.fd = events[i].data.fd;
                ev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_MOD,events[i].data.fd,&ev);
            } else {
                int k = connect_participant_socket_pos[events[i].data.fd];
                succeed_participant.erase(k);
                failed_participant.insert(k);
                participants_commit_ID[connect_participant_socket_pos[events[i].data.fd]] = -1;
                close(events[i].data.fd);
            }
        }
        if(num == n){
            t = 0;
        }
        if(t == 1)
            usleep(timeout);
    }
    recovery();
    delete events;
    delete buf;
    return res; 
}
void coordinator::recovery()
{
    while(1) {
        if(participants_commit_ID.size() == 0) return;
        int maxID = -1;
        for(int i = 0; i < participants_commit_ID.size(); i++) {
            if(participants_commit_ID[i] > maxID)
                maxID = participants_commit_ID[i];
        } 
        if(maxID == -1) return;
        queue <int> good;
        queue <int> bad;
        for(int i = 0; i < participants_commit_ID.size(); i++) {
            if(participants_commit_ID[i] == -1) continue;
            if(participants_commit_ID[i] == maxID) good.push(i);
            else bad.push(i);
        }
        if(bad.empty()) return;
        while(1) {
            int k = bad.front();
            send_message(good.front(), "&");
            string s = get_message(good.front());
            if(s == "") {
                if(succeed_participant.find(good.front()) != succeed_participant.end()) {
                    succeed_participant.erase(good.front());
                    failed_participant.insert(good.front());
                    close(connect_participant_pos_socket[good.front()]);
                }
                participants_commit_ID[good.front()] = -1;
                good.pop();
                if(good.size() == 0) break;
            }else {
                send_message(k, s);
                s = get_message(k);
                if(s == "") {
                    if(succeed_participant.find(k) != succeed_participant.end()) {
                        succeed_participant.erase(k);
                        failed_participant.insert(k);
                        close(connect_participant_pos_socket[k]);
                    }
                    participants_commit_ID[k] = -1;
                }else {
                    participants_commit_ID[k] = maxID;
                }
                bad.pop();
                if(bad.size() == 0) return;
            }
        }
    }
}
coordinator::~coordinator()
{
}