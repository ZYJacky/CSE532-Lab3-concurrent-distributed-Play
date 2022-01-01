/*
 *   Stream_Handler.cpp    
 *
 *   Implements member functions declared in Stream_Handler.h
 *   
 *   
 *   Created: 12/11/21
 *   Last edited: 12/11/21
 */

#include "Stream_Handler.h"

#include <iostream>

// constructor
Stream_Handler::Stream_Handler(ACE_SOCK_Stream* _stream, unsigned int _id, ACE_Reactor* _reactor, std::vector<bool>* _available, unsigned int* _num_stream, std::vector<Stream_Handler*>* _stream_handlers)
                        : stream(_stream), director_id(_id), reactor(_reactor), available(_available), flag(true), num_stream(_num_stream), stream_handlers(_stream_handlers), in_progress(FREE)
{}
        
// overriden default destructor
Stream_Handler::~Stream_Handler()
{
    flag = false;
}

// overriden handle_close method
int Stream_Handler::handle_close (ACE_HANDLE handle, ACE_Reactor_Mask mask)
{   
    stream->close();
    delete stream;
    
    if(flag) delete this;

    return SUCCESS;
}

// return handle of the SOCK_STREAM
ACE_HANDLE Stream_Handler::get_handle(void) const
{
    return stream->get_handle();
}

// handle input from Director
int Stream_Handler::handle_input (ACE_HANDLE h)
{   
    char buf[BUF_SIZE];

    int recv_byte;
    if((recv_byte = stream->recv(buf, BUF_SIZE)) == 0)
    {   
        // Director closes stream
        std::cout << std::endl << "Director " << director_id << " goes offline" << std::endl;

        (*available)[director_id] = false;
        --(*num_stream);

        print_play_list();

        reactor->remove_handler(this, ACE_Event_Handler::READ_MASK);

        return SUCCESS;
    }
    
    // there are some wired char displayed if constructing
    // string directly from buffer; do it this way eliminates that
    std::string msg;
    for(int i = 0; i < recv_byte; ++i)  msg += buf[i];

    std::istringstream iss (msg);
    std::string type;

    iss >> type;

    if(type.compare("done") == 0) // director finishes playing, mark it free
    {
        in_progress = FREE;
    }
    else if(type.compare("started") == 0) // director started a play
    {
        unsigned int play_num;
        iss >> play_num;
        in_progress = play_num;
    }
    else  // director is reporting its plays
    {
        std::istringstream iss (msg);

        std::string play_name;
        while(iss >> play_name)
        {   
            play_names.push_back(play_name);
        }
    }

    print_play_list();


    return SUCCESS;
}

// print play list in all stream_handlers
void Stream_Handler::print_play_list(void)
{
    std::cout << std::endl << "-------------" << std::endl << "[List of Plays]" << std::endl;
    for(unsigned int i = 0; i < (*stream_handlers).size(); ++i)
    {   
        if((*available)[i]) (*stream_handlers)[i]->print_files();
    }
    std::cout << "-------------" << std::endl;
}

// give access to private variable files in each stream
void Stream_Handler::print_files(void)
{   
    std::cout << std::endl << "Director " << director_id << std::endl;
    for(int i = 0; i < (int)play_names.size(); ++i)
    {   
        std::cout << i << " " << play_names[i];
        if(i == in_progress) std::cout << " [in progress]" << std::endl;
        else if(in_progress >= 0) std::cout << " [unavailable]" << std::endl;
        else std::cout << " [available]" << std::endl;
    }
}

// tells the director to stop a play
int Stream_Handler::stop_a_play(unsigned int play_num)
{   
    // play not currently in progress
    if(in_progress != (int)play_num)
    {   
        std::cout << "warning: stopping play fails as play is not in progress." << std::endl;
        return NO_PLAY;
    }

    const char* buf = "stop";
    if(stream->send_n(buf, strlen(buf)) <= 0)
    {
        std::cout << "warning: stopping play" << play_num << " to director " << director_id << "fail" << std::endl;
        return SEND_FAIL; 
    }

    return SUCCESS;
}

// tells the director to start a play
int Stream_Handler::ask_for_play(unsigned int play_num)
{   
    // other play in progress, can't stop
    if(in_progress >= 0 || play_num > (play_names.size() - 1))
    {
        return NO_PLAY;
    }
    
    const char* buf = std::to_string(play_num).c_str();
    if(stream->send_n(buf, strlen(buf)) <= 0)
    {
        std::cout << "warning: request for play " << play_num << " to director " << director_id << "fail" << std::endl;
        return SEND_FAIL; 
    }

    return SUCCESS;
}