/*
 *   Stream_Handler.h     
 *
 *   Definition of class Stream_Handler that inherits from ACE_Event_Handler
 *   and handle input over ACE_SOCK_STREAM
 *   
 *   Created: 12/9/21
 *   Last edited: 12/11/21
 */

#ifndef STREAM_HANDLER_H
#define STREAM_HANDLER_H

#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"

#include "utility.h"

#include <sstream>
#include <vector>

#define FREE -1

#ifndef BUF_SIZE
    #define BUF_SIZE 1024
#endif

class Stream_Handler : public ACE_Event_Handler
{   
    public:

        Stream_Handler(ACE_SOCK_Stream* _stream, unsigned int _id, ACE_Reactor* _reactor, std::vector<bool>* _available, unsigned int* _num_stream, std::vector<Stream_Handler*>* _stream_handlers);
        virtual ~Stream_Handler();

        virtual int handle_close (ACE_HANDLE handle, ACE_Reactor_Mask mask);
        virtual ACE_HANDLE get_handle(void) const;
        virtual int handle_input (ACE_HANDLE h = ACE_INVALID_HANDLE);

        void print_play_list(void);
        void print_files(void);
        
        int stop_a_play(unsigned int play_num);
        int ask_for_play(unsigned int play_num);

    private:

        ACE_SOCK_Stream* stream;  
        unsigned int director_id;
        ACE_Reactor* reactor;
        std::vector<bool>* available;   // keep track of whether a stream is connected or not
        bool flag; 
        unsigned int* num_stream;
        std::vector<std::string> play_names;   // names of play of this director
        std::vector<Stream_Handler*>* stream_handlers;  
        int in_progress;           // keep track of which play if any is in progress
};

#endif // Stream_Handler_H