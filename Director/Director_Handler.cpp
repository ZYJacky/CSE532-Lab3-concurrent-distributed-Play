/*
 *   Director_Handler.cpp    
 *
 *   Implements member functions declared in Director_Handler.h
 *   
 *   
 *   Created: 12/11/21
 *   Last edited: 12/11/21
 */

#include "Director_Handler.h"

// Default constructor
Director_Handler::Director_Handler(ACE_Reactor* _reactor, ACE_SOCK_Stream& _stream, Director* _director)
                            : reactor(_reactor), stream(_stream), director(_director)
{};

// get handle from ACE_SOCK_STREAM
ACE_HANDLE Director_Handler::get_handle(void) const
{
    return stream.get_handle();
}

// handle input
int Director_Handler::handle_input (ACE_HANDLE)
{   
    char buf[BUF_SIZE];
    stream_lk.lock();
    if(stream.recv(buf, BUF_SIZE) == 0)
    {   
        // tells director to stop
        director->end_playing();
        if(t.joinable()) t.join();

        std::cout << "Connection with Producer terminates." << std::endl;

        stream.close();
        reactor->end_reactor_event_loop();
        reactor->close();

        stream_lk.unlock();

        return SUCCESS;
    }

    stream_lk.unlock();

    std::string msg (buf);
    std::istringstream iss (msg);

    unsigned int play_num;
    if(iss >> play_num)
    {   
        // start reciting; since the handler still needs to respond 
        // during reciting, lauch this in another thread

        // clean up previous run that must be finished at this point since Stream_Handler will only
        // send this call if the Director has sent "done" to it
        if(t.joinable()) t.join(); 

        t = std::move(std::thread(&Director::cue, director, play_num, &stream, &stream_lk));
        
        
        stream_lk.lock();
        std::string str = "started " + std::to_string(play_num);
        if(stream.send_n(str.c_str(), str.length()) <= 0)
        {
            std::cout << "warning: reporting to Producer unsuccessful." << std::endl;
        }
        stream_lk.unlock();
    }
    else
    {   
        std::cout << "stop received" << std::endl;
        director->stop_playing();
        if(t.joinable()) t.join();
    }

    return SUCCESS;
}

// handle SIGINT/Ctrl+C
int Director_Handler::handle_signal (int, siginfo_t*, ucontext_t*)
{   
    director->end_playing();
    if(t.joinable()) t.join();

    stream.close();  // close the stream to let Producer know

    reactor->end_reactor_event_loop();
    reactor->close();

    std::cout << "Director out." << std::endl;

    return SUCCESS;
}