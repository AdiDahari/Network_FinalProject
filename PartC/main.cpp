#include "Node.hpp"
#include "select.h"
#include "Message.hpp"
#include <string>
#include <iostream>
using namespace std;
using namespace Network;
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "please run the main in the following order: ./main <port-number>" << endl;
        exit(0);
    }
    int port = stoi(argv[1]);
    Node n{port};
    n.run();
}