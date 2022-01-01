/*
 *  main.cpp     CSE532S Lab 3 version
 *
 *  The entry point of the program "Producer". 
 *  
 *  Created: 9/16/21
 *  Last edited: 12/8/21
 */

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "ace/Process.h"

#include <iostream> // for std::cout, std::endl
#include <sstream> // for std::istringstream

#include "Producer.h"
#include "utility.h"
#include "IO_Handler.h"

#ifndef BUF_SIZE
    #define BUF_SIZE 1024
#endif

// program entry point
int main(int argc, char* argv[])
{   
    // check argc
    if(argc < MIN_ARG_NEEDED)
    {
        std::cout << "usage: " << argv[PROGRAM_NAME] << "[port]" << std::endl;
        return INVALID_ARG;
    }

    // try getting a port number
    unsigned short port_num;
    std::istringstream iss (argv[PORT_NUM]);
    if(!(iss >> port_num))
    {
        std::cout << "usage: " << argv[PROGRAM_NAME] << "[port]" << std::endl;
        return INVALID_ARG;
    }

    // try binding port
    ACE_INET_Addr addr(port_num, ACE_LOCALHOST);
    ACE_SOCK_Acceptor acceptor;

    if(acceptor.open(addr, 1) < 0)
    {
        std::cout << "port binding fails, please try another port" << std::endl;
        return PORT_OPEN_FAIL;
    }

    // usage message
    ACE_TCHAR server_addr[BUF_SIZE];
    if(addr.addr_to_string(server_addr, sizeof server_addr) == 0)
    {
        std::cout << std::endl << "Producer listening on: " << server_addr << std::endl;
        std::cout << "To start a play: start [director number] [play number]" << std::endl; 
        std::cout << "To stop a play: stop [director number] [play number]" << std::endl; 
        std::cout << "To quit: quit" << std::endl; 
    }

    // construct reactor and producer instance
    ACE_Reactor* reactor = ACE_Reactor::instance();
    Producer producer(acceptor, reactor);

    // register connection handler
    if(reactor->register_handler(&producer, ACE_Event_Handler::ACCEPT_MASK) != SUCCESS)
    {   
        std::cout << "Registering producer with ACCEPT_MASK failed." << std::endl;
        return REGISTER_FAIL;
    }

    // register signal handler
    if(reactor->register_handler(SIGINT, &producer) != SUCCESS)
    {   
        std::cout << "Registering producer with ACCEPT_MASK failed." << std::endl;
        return REGISTER_FAIL;
    }

    // register io handler
    IO_Handler io_handler (&producer);
    if(reactor->register_handler(&io_handler, ACE_Event_Handler::READ_MASK) != SUCCESS)
    {   
        std::cout << "Registering producer with ACCEPT_MASK failed." << std::endl;
        return REGISTER_FAIL;
    }
    
    // start reactor
    reactor->run_reactor_event_loop();

    return SUCCESS;
}