/*
 *  IO_Handler.cpp     
 *
 *  Declare class IO_Handler that inherite from ACE_Event Handler
 *  and is responsible for handling std::cin events.
 *  
 *  Created: 12/10/21
 *  Last edited: 12/11/21
 */

#ifndef IO_HANDLER_H
#define IO_HANDLER_H

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "ace/Process.h"

#include "Producer.h"
#include "utility.h"

#include <iostream> // for std::cout, std::endl
#include <sstream> // for std::istringstream

class IO_Handler : public ACE_Event_Handler
{   
    private:

        Producer* producer;

    public:

        IO_Handler(Producer* _producer);

        virtual ACE_HANDLE get_handle(void) const;
        virtual int handle_input (ACE_HANDLE h = ACE_INVALID_HANDLE);
};

#endif // IO_HANDLER_H