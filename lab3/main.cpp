#include "kvstore2pcsystem.h"
#include "coordinator.h"
#include "participant.h"
#include <iostream>
using namespace std;
int main(int argc, char * argv[])
{
    kvstore2pcsystem KVS2PC(argc, argv);
    cout << KVS2PC.getfilename() << endl; 
    cout << "mode = " << KVS2PC.getmode() << endl;
    if(KVS2PC.getmode() == 0) {
        coordinator CDT(KVS2PC.getfilename());
        CDT.start();
    }
    else if(KVS2PC.getmode() == 1) {
        participant PTP(KVS2PC.getfilename());
        PTP.start();
    }
    else {
        perror("mode error!!!");
        exit(0);
    }
}