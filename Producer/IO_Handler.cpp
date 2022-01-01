/*
 *  IO_Handler.cpp     
 *
 *  Implements member functions from IO_Handler.h
 *  
 *  Created: 12/11/21
 *  Last edited: 12/11/21
 */

#include "IO_Handler.h"

// constructor
IO_Handler::IO_Handler(Producer* _producer)
                        : producer(_producer)
{}

// overriden get_handle method; return handle to stdin
ACE_HANDLE IO_Handler::get_handle(void) const
{
    return ACE_STDIN;
}

// overriden handle_input method; pass command to producer
// which has all the needed private variables to react
int IO_Handler::handle_input (ACE_HANDLE h)
{      
    std::string command;
    std::getline(std::cin, command);

    producer->process_command(command);
                
    return SUCCESS;
}