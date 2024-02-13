#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>
#define CE_SERIAL_IMPLEMENTATION
#include "ceSerial.h"
#include "windows.h"
#include<fstream>



int main()
{
    //open port
    ceSerial com("\\\\.\\COM5", 9600, 8, 'N', 1);
    printf("Opening port %s.\n", com.GetPort().c_str());

    //keep opening port until port on
    while ((com.Open() != 0))
    {
        continue;
    }
    std::cout << "success!" << std::endl;

    bool successflag;

    //get controller state from serialport and write it to file
    while (true)
    {
        char c[] = { com.ReadChar(successflag) };
        std::cout << c[0] << std::endl;
        
    }
    com.Close();
    return 0;
}


