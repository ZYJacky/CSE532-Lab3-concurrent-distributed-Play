/*
 *   Director_Hander.h     
 *
 *   Definition of class Director Handler that inherits from ACE_Event_Handler
 *   and handle SOCK_Stream event and SIGINT event.
 *   
 *   Created: 12/5/21
 *   Last edited: 12/11/21
 */

#ifndef DIRECTOR_HANDLER_H
#define DIRECTOR_HANDLER_H

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"

#include "Director.h"
#include "utility.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>

#define BUF_SIZE 256

class Director_Handler : public ACE_Event_Handler
{
    public:

        Director_Handler(ACE_Reactor* _reactor, ACE_SOCK_Stream& _stream, Director* _director);

        virtual ACE_HANDLE get_handle(void) const;
        virtual int handle_input (ACE_HANDLE h = ACE_INVALID_HANDLE);
        virtual int handle_signal (int, siginfo_t* = 0, ucontext_t* = 0);

    private:

        ACE_Reactor* reactor;
        ACE_SOCK_Stream stream;
        Director* director;
        std::mutex stream_lk;

        std::thread t;
};

#endif // DIRECTOR_HANDLER_H