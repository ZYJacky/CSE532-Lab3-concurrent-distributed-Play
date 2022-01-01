/*
 *   Player.cpp
 *
 *   Implement member functions declared in Player.h
 *
 *   Created: 10/6/21
 *   Last edited: 10/8/21
 */

#include "Player.h"
#include "utility.h"

#include <sstream> // for istringstream
#include <iostream>

Player::Player(std::vector<std::shared_ptr<Play>>& _play, threadsafe_queue<std::string>& _work_queue, std::atomic<bool>* _should_stop)
                : plays(_play), work_queue(_work_queue), should_stop(_should_stop)
{}

// lanuch worker thread
void Player::enter(unsigned int play_id)
{   
    worker = std::move(std::thread(&Player::act, this, play_id));
}

// repeatedly pull from work queue and process job
void Player::act(unsigned int play_id)
{      
    while(1)
    {
        std::string part_def;
        work_queue.wait_and_pop(part_def);

        if(part_def.compare("end") == 0) 
        {   
            work_queue.push("end");   // re-push end token to let other players know
            break;
        }

        std::istringstream iss (part_def);
        unsigned int role_count;
        unsigned int frag_count;
        std::string role_name;
        std::string role_file_name;
        
        // extract all info
        iss >> role_count;
        iss >> frag_count;
        iss >> role_name;
        iss >> role_file_name;

        // if the provided part file cannot be open, throw a warning and ignore
        std::ifstream role_file (role_file_name);
        read(role_file, role_name);

        // enter will block till player is in the right scene
        int ret_val = plays[play_id]->enter(frag_count, role_count);
        if(*should_stop)
        {   
            plays[play_id]->notify_all_cv();
            break;
        }

        if(ret_val != SUCCESS) continue;

        std::vector<structured_line>::iterator it = lines.begin();    // iterator at the start of the lines
        std::vector<structured_line>::iterator end = lines.end();    // iterator at the start of the lines
        while(it != end)
        {   
            //std::cin.ignore();        // intentionlly left commented for testing stop function 
            if(*should_stop) break;
            plays[play_id]->recite(it, end);
        }
        if(*should_stop)
        {   
            plays[play_id]->notify_all_cv();
            break;
        }
    } 
}

// join joinable thread
void Player::exit(void)
{   
    if(worker.joinable()) worker.join();
}

// read the input file given at construction and fill the structured lines container
void Player::read(std::ifstream& in_file, std::string PlayerName)
{   
    lines.clear();
    while(!in_file.eof())
    {   
        std::string line, words;
        unsigned int LineNo;

        std::getline(in_file, line);
        if(line.length() == 0) continue;  // skip empty line

        std::istringstream iss(line);
        if(!(iss >> LineNo)) continue;   // can't extract a line number

        if(!(iss >> words)) continue;  // nothing after a line number

        // get and concatenate rest of the string
        std::string unread = iss.eof() ? "" : iss.str().substr( iss.tellg() );
        words += unread;
        
        lines.push_back(std::make_pair(std::make_pair(LineNo, PlayerName), words));
    }                                                                           
}

