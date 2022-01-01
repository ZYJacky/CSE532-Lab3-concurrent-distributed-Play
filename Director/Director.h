/*
 *   Director.h     Version: CSE532S Lab3
 *
 *   Declaration of class Director, that represents a director in the play who is responsible
 *   for constructing Play and Players and handing part script to them  
 *
 *   Created: 11/1/21
 *   Last edited: 11/8/21
 */

#ifndef DIRECTOR_H
#define DIRECTOR_H

#include "Play.h"
#include "Player.h"
#include "ace/SOCK_Stream.h"

#include <vector>
#include <string>
#include <memory> // for shared_ptr
#include <mutex>
#include <atomic>



class Director
{   
    public:

        Director(std::vector<std::string> file_names, unsigned int min_thread);

        void cue(unsigned int play_id, ACE_SOCK_STREAM* stream, std::mutex* stream_lk);
        void reset(unsigned int play_num);
        void stop_playing(void);
        void end_playing(void);

    private:

        std::vector<std::vector<std::string>> SceneNames;
        std::vector<std::vector<std::string>> part_defs;

        std::shared_ptr<threadsafe_queue<std::string>> script_queue;
        std::vector<std::shared_ptr<Play>> shared_plays;
        std::vector<std::shared_ptr<Player>> shared_players; 

        std::atomic<bool> should_stop;
        std::atomic<bool> should_end;
};

#endif // DIRECTOR_H