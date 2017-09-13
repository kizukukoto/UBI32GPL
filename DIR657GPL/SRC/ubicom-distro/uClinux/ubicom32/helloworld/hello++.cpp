#include <iostream>
#include "stdio.h"
using namespace std;
 
class Main
{
public:
        Main();
        ~Main();
        virtual void run() = 0;
};
 
class MyMain : public Main
{
public:
        virtual void run();
};
 
Main::Main()
{
        cout << "Main::Main()" << endl;
}
 
Main::~Main()
{
        cout << "Main::~Main()" << endl; }
 
void MyMain::run()
{
        cout << "MyMain::run()" << endl; }
 
MyMain app_mymain;
Main *app_main = &app_mymain;
 
int main(int argc, char *argv[])
{
        printf("Starting Helloworld Test Now \n");
        printf(__DATE__ " " __TIME__ " \n");
 
        app_main->run();
        cout << "Hello World" << endl;
        return 0;
}
