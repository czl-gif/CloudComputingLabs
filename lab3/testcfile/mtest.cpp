#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
using namespace std;
int main()
{
    string s = "#8\r\ncs\r\ncd ll\r\nfd\r\njk kk\r\n";
    stringstream ss(s);
    string s1,s2;
    ss >> s1 ;cout << s1 << endl;
    ss >> s1; cout << s1 << endl;
    getline(ss, s1);
    getline(ss, s2); cout << s2 << endl;
}
