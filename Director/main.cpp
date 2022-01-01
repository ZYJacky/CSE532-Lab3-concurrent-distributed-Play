/*
 *  main.cpp     CSE532S Lab 3 version
 *
 *  The entry point of the program "Director". It checks the validity of command line arguments
 *  and construct a Director passing it the approriate argument. It calls the director's
 *  cue() method to recite the play.
 *  
 *
 *  Created: 9/16/21
 *  Last edited: 12/5/21
 */

// local header files
#include "Director.h"
#include "utility.h"
#include "Director_Handler.h"

// standard library
#include <sstream>  // for std::istringstream
#include <iostream> // for cout, endl
#include <vector>   // for std::vector
#include <stdlib.h>     //for using the function sleep

// ACE library
#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"


// entry point of the program
int main(int argc, char* argv[])
{
    // see if meet minimum number of arguments
    if(argc < MIN_ARGS_NEEDED)
    {
        std::cout << "usage: " << argv[0] << " <port> <ip_address> <min_threads> <script_file>+" << std::endl;
        return INVALID_ARG;
    }

    //verifying arguments
    unsigned int min_thread;
    std::istringstream iss_min_thread (argv[MIN_THREAD]);
    if(!(iss_min_thread >> min_thread))
    {
        std::cout << "usage: " << argv[0] << " <port> <ip_address> <min_threads> <script_file>+" << std::endl;
        return INVALID_ARG;
    }

    //verifying arguments
    unsigned short port_num;
    std::istringstream iss_port_num (argv[PORT_NUM]);
    if(!(iss_port_num >> port_num))
    {
        std::cout << "usage: " << argv[0] << " <port> <ip_address> <min_threads> <script_file>+" << std::endl;
        return INVALID_ARG;
    }
    
    // TODO: what to do with -override? 

    // extract all script files names
    std::vector<std::string> file_names;
    for(int i = FILE_START; i < argc; ++i)
    {   
        std::ifstream ifs (argv[i]);
        if(!ifs.is_open())
        {
            std::cout << "warning: " << argv[i] << " not found and is ignored." << std::endl;
            continue;
        }
        file_names.push_back(argv[i]);
    }

    // no at least one valid file
    if(file_names.size() == 0)
    {
        std::cout << "Cannot find at least one valid file. Program terminates." << std::endl;
        return NO_FILE;
    }

    try
    {
        Director director (file_names, min_thread);
        
        ACE_INET_Addr addr(port_num, argv[IP_ADDR]);
        ACE_SOCK_Connector connector;
        ACE_SOCK_Stream stream;


        // establish connection to Producer
        if(connector.connect(stream, addr) < 0)
        {
            std::cout << "Connection fail, please check IP address and port number" << std::endl;
            return CONNECT_FAIL; 
        }

        ACE_Reactor* reactor = ACE_Reactor::instance();
        Director_Handler director_handler(reactor, stream, &director);

        if(reactor->register_handler(&director_handler, ACE_Event_Handler::READ_MASK) != SUCCESS)
        {   
            stream.close();
            return REGISTER_FAIL;
        }

        if(reactor->register_handler(SIGINT, &director_handler) != SUCCESS)
        {   
            stream.close();
            return REGISTER_FAIL;
        }
        
        // send initial play list to Producer
        std::string msg;
        for(unsigned int i = 0; i < file_names.size(); ++i)
        {   
            msg += file_names[i];
            if(i != file_names.size() - 1) msg += " ";
        }
        if(stream.send_n(msg.c_str(), msg.length()) <= 0)
        {
            std::cout << "warning: can't send file names to Producer. Suggest restarting the program" << std::endl;
        }  

        // start event loop
        reactor->run_reactor_event_loop();
    }
    catch(RET_VALUES error_val)
    {   
        return error_val;
    }
    catch(...)
    {
        std::cout << "Unknown failure not explicitly expected by the program is thrown" << std::endl;
        return UNKNOWN_ERROR;
    }

    return SUCCESS;
}