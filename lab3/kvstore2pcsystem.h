#ifndef KVSTORE2PCSYSTEM_H_INCLUDE
#define KVSTORE2PCSYSTEM_H_INCLUDE
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
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
class kvstore2pcsystem
{
private:
    int mode;
    char* filename;
    void init();
public:
    char* getfilename() {return filename;}
    int getmode() {return mode;}
    kvstore2pcsystem(int argc, char* argv[]);
    ~kvstore2pcsystem();
};





#endif