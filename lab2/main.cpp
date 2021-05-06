#include<iostream>
#include "httpserver.h"
using namespace std;

int main(int argc, char * const argv[])
{
    init(argc, argv);
    //cout << "ip = " << ip << " port = " << port << "number-thread = " << number_thread << endl;
    Start();
}