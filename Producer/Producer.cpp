/*
 *   Producer.cpp    
 *
 *   Implements member functions declared in Producer.h
 *   
 *   
 *   Created: 12/11/21
 *   Last edited: 12/11/21
 */

#include "Producer.h"

#include <iostream> // for std::cout, std::endl
#include <sstream>  // for istringstream

// constructor
Producer::Producer(ACE_SOCK_Acceptor& _acceptor, ACE_Reactor* _reactor)
            : acceptor(_acceptor), reactor(_reactor), num_stream(0) 
{};

// overriden get_handle that return handle to the acceptor
ACE_HANDLE Producer::get_handle(void) const
{
    return acceptor.get_handle();
}

// overriden handle_input; handle connection and construct Stream_Handler object
int Producer::handle_input (ACE_HANDLE h)
{   
    // accept stream
    ACE_SOCK_Stream* stream = new ACE_SOCK_Stream;  
    if(acceptor.accept(*stream) != SUCCESS)
    {
        std::cout << "warning: accepting stream fails" << std::endl;
        return NO_CONNECTION;
    }

    // determine director's id
    int director_id = 0;
    if(num_stream == stream_handlers.size()) 
    {
        director_id = num_stream;
        stream_handlers.push_back(new Stream_Handler(stream, director_id, reactor, &available, &num_stream, &stream_handlers));
        available.push_back(true);
    }
    else
    {   
        for(unsigned int i = 0; i < available.size(); ++i)
        {   
            // replace disconnected socket
            if(available[i] == false)
            {
                director_id = i;   
                stream_handlers[i] = new Stream_Handler(stream, director_id, reactor, &available, &num_stream, &stream_handlers);
                available[i] = true;
                break;
            }
        }
    }

    // register stream handler
    if(reactor->register_handler(stream_handlers[director_id], ACE_Event_Handler::READ_MASK) != SUCCESS)
    {
        std::cout << "warning: director registration fail. Clean up resource." << std::endl;
        stream_handlers[director_id] = nullptr;
        delete stream;
        stream->close();
        return REGISTER_FAIL;
    }

    ++num_stream;
    std::cout << std::endl << "Director " << director_id << " now online." << std::endl;

    return SUCCESS;
}

// overriden singal; react to Ctrl_C or SIGINT by closing the reactor
int Producer::handle_signal (int i, siginfo_t* s1, ucontext_t* s2)
{   
    reactor->end_reactor_event_loop(); // all handlers will be unregistered
    reactor->close();

    return SUCCESS;
}

// process command that is received in std::in
void Producer::process_command(std::string command)
{
    std::istringstream iss (command);
    std::string command_type;
    iss >> command_type;
                
    if(command_type.compare("start") == 0)
    {
        unsigned int director_num;
        if(!(iss >> director_num))
        {
            std::cout << "To start a play: start [director number] [play number]" << std::endl; 
            return;
        }

        if(director_num > stream_handlers.size() || available[director_num] == false)
        {
            std::cout << "Invalid director number, please check Play list." << std::endl; 
            return;
        }

        unsigned int play_num;
        if(!(iss >> play_num))
        {
            std::cout << "To start a play: start [director number] [play number]" << std::endl; 
            return;
        }


        // ask for a play
        if(stream_handlers[director_num]->ask_for_play(play_num) != SUCCESS)
        {
            std::cout << "Play unavailable, please check Play list." << std::endl; 
            return;
        }
    }
    else if(command_type.compare("stop") == 0)
    {
        unsigned int director_num;
        if(!(iss >> director_num))
        {
            std::cout << "To stop a play: stop [director number] [play number]" << std::endl; 
            return;
        }

        if(director_num > stream_handlers.size() || available[director_num] == false)
        {
            std::cout << "Invalid director number, please check Play list." << std::endl; 
            return;
        }

        unsigned int play_num;
        if(!(iss >> play_num))
        {
            std::cout << "To stop a play: stop [director number] [play number]" << std::endl; 
            return;
        }

        // stop a play
        if(stream_handlers[director_num]->stop_a_play(play_num) != SUCCESS)
        {
            std::cout << "Play cannot be stop, please check Play list." << std::endl; 
            return;
        }
    } 
    else if(command_type.compare("quit") == 0)
    {   
        reactor->end_reactor_event_loop();
        reactor->close();
    }
    else
    {
        std::cout << std::endl << "command not recognized" << std::endl;
        std::cout << "To start a play: start [director number] [play number]" << std::endl; 
        std::cout << "To stop a play: stop [director number] [play number]" << std::endl; 
        std::cout << "To quit: quit" << std::endl; 
    }
}