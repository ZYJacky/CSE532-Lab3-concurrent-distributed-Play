/*
 *   Producer.h     
 *
 *   Definition of class Producer that inherits from ACE_Event_Handler
 *   and handle connection request.
 *   
 *   Created: 12/5/21
 *   Last edited: 12/11/21
 */

#ifndef PRODUCER_H
#define PRODUCER_H

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"

#include <iostream> // for std::cout, std::endl
#include <vector>

#include "Stream_Handler.h"
#include "utility.h"


class Producer : public ACE_Event_Handler
{   
    private:

        ACE_SOCK_Acceptor acceptor;
        ACE_Reactor* reactor;

        std::vector<Stream_Handler*> stream_handlers;
        std::vector<bool> available;
        unsigned int num_stream;


    public:

        Producer(ACE_SOCK_Acceptor& _acceptor, ACE_Reactor* _reactor);

        virtual ACE_HANDLE get_handle(void) const;
        virtual int handle_input (ACE_HANDLE h = ACE_INVALID_HANDLE);
        virtual int handle_signal (int i, siginfo_t* s1 = 0, ucontext_t* s2 = 0);

        void process_command(std::string command); 
};

#endif // PRODUCER_H