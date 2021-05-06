#include<iostream>
#include "httpserver.h"
#include "proxyserver.h"
#include <string.h>
using namespace std;

int main(int argc, char * const argv[])
{
    init(argc, argv);
    //cout << "ip = " << ip << " port = " << port << "number-thread = " << number_thread << endl;
    if (strlen(proxy) == 0)
        Start();
    else 
        proxyStart();
}