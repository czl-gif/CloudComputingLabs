#include <iostream>
#include <thread>
#include <vector>
using namespace std;

void threadfun1()
{
	cout << "threadfun1 start \r\n";
	this_thread::sleep_for(chrono::seconds(1));
	cout << "threadfun1 end \r\n";
}
 
void threadfun2(int iParam, string sParam)
{
	string paramIn;
	paramIn = to_string(iParam) + sParam;
	cout << "threadfun2 start " << paramIn << endl;
	this_thread::sleep_for(chrono::seconds(5));
	cout << "threadfun2 end\r\n";
}

int main()
{
    vector<thread> t;
    for(int i = 1; i < 5; i++) {
        //thread t1(threadfun1);
        t.push_back(move(thread(threadfun2, 1, "tttt")));
        
    }
    for(int i = 0; i < 4; i++) {
        t[i].join();
        cout << "lala" << endl;
    }
    //thread t2()
}