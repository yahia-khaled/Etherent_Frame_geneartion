#include <iostream>
#include <fstream>
#include "Ethernet.h"

using namespace std;



int main(int argc, char)
{
    Ethernet net;

    net.ReadConfig("first_milestone.txt");

    net.createoutput();

    system("pause");

    return 0;
}
