#include "kvstoreraftsystem.h"
#include <iostream>
using namespace std;
int main(int argc, char* argv[])
{
    kvstoreraftsystem raft(argc, argv);
    raft.start();
}