/*
 *   Director.cpp    Version: CSE532S Lab3
 *
 *   Implements member functions declared in Director.h
 *
 *   Created: 11/1/21
 *   Last edited: 12/6/21
 */

#include "Director.h"
#include "Play.h"
#include "utility.h"
#include "ace/SOCK_Stream.h"

#include <fstream>
#include <sstream>
#include <iostream>

void Director::stop_playing()
{   
    should_stop = true;
}

void Director::end_playing()
{   
    should_stop = true;
    should_end = true;
}

// reset state of play
void Director::reset(unsigned int play_num)
{   
    script_queue->clear();
    should_stop = false;
    shared_plays[play_num]->reset();
}

// parse all script files and construct Play and Player
Director::Director(std::vector<std::string> file_names, unsigned int min_thread)
{   
    unsigned int valid_file_count = 0;
    unsigned int max_part_count = 0; 
    should_stop = false;
    should_end = false;

    for(unsigned int i = 0; i < file_names.size(); ++i)
    {
        std::ifstream ifs(file_names[i]);

        SceneNames.push_back({});   // allocate a slot for this play
        part_defs.push_back({});

        unsigned int prev_part_count = 0;
        bool prev_is_config = false;
        unsigned int frag_count = 1; // count fragment/config file

        while(!ifs.eof())
        {   
            std::string line;
            std::getline(ifs, line);

            std::istringstream iss (line);
            std::string word;
            if(!(iss >> word)) continue;  //skip empty line

            if(word.compare("[scene]") == 0)
            {   
                std::string tmp;
                iss >> tmp;                // extract the first word after [scene]
                std::string unread = iss.eof() ? "" : iss.str().substr( iss.tellg() );  // get and concatenate rest of the string
                tmp = tmp + unread;
                SceneNames[valid_file_count].push_back(tmp);  // store the scene name

                prev_is_config = false;
            } 
            else
            {
                std::ifstream config_file (word);
                if(config_file.is_open())
                {   
                    std::vector<std::string> tmp_part_defs;
                    unsigned int part_count = 0;
                    while(!config_file.eof())
                    {
                        std::string config_line;
                        std::getline(config_file, config_line);
                    
                        // ignore empty and blank line
                        std::string role_name;
                        std::istringstream config_iss (config_line);
                        if(!(config_iss >> role_name))  continue;

                        // can we get a file name?
                        std::string role_file_name;
                        if(!(config_iss >> role_file_name))  continue;

                        // check if role file can be open
                        std::ifstream role_file (role_file_name);
                        if(!role_file.is_open()) continue;

                        tmp_part_defs.push_back(std::to_string(frag_count) + " " + role_name + " " + role_file_name);

                        ++part_count;
                    }

                    for(unsigned int i = 0; i < tmp_part_defs.size(); ++i)
                    {
                        std::string tmp = std::to_string(part_count) + " " + tmp_part_defs[i];
                        part_defs[valid_file_count].push_back(tmp);
                    }

                    if(prev_is_config) SceneNames[valid_file_count].push_back("-----Same scene, but stage and characters switched.-----");

                    if(part_count + prev_part_count > max_part_count) max_part_count = part_count + prev_part_count;

                    prev_part_count = part_count;
                    prev_is_config = true;

                    ++frag_count; 
                }
            }
        }
        ++valid_file_count;
    }

    // calculate thread_pool size
    unsigned int player_pool_size = std::max(min_thread, max_part_count);

    for(unsigned int i = 0; i < valid_file_count; ++i)
    {   
        shared_plays.push_back(std::move(std::make_shared<Play>(SceneNames[i], player_pool_size, &should_stop)));
    }

    script_queue = std::move(std::make_shared<threadsafe_queue<std::string>>());

    for(unsigned int i = 0; i < player_pool_size; ++i)
    {   
        shared_players.push_back(std::make_shared<Player>(shared_plays, *script_queue, &should_stop));
    }
}

// ask players to enter to start a play and distribute part definitions
void Director::cue(unsigned int play_id, ACE_SOCK_STREAM* stream, std::mutex* stream_lk)
{   
    // To prevent an extreme scenario of user sending stop command, and before
    // that command is received and processed the play ends
    should_stop = false;

    // ask players to enter
    for(unsigned int i = 0; i < shared_players.size(); ++i)
    {   
        shared_players[i]->enter(play_id);
    }

    // submitting jobs to the queue
    for(unsigned int i = 0; i < part_defs[play_id].size(); ++i)
    {   
        if(should_stop) break;
        script_queue->push(part_defs[play_id][i]);
    }

    script_queue->push("end"); // give end token 

    // ask players to exit
    for(unsigned int i = 0; i < shared_players.size(); ++i)
    {
        shared_players[i]->exit();
    }

    // reset Play status
    reset(play_id);

    if(!should_end)
    {
        // tell Producer play has finished
        stream_lk->lock();
        std::string str ("done");
        if(stream->send_n(str.c_str(), str.length()) <= 0)
        {
            std::cout << "warning: reporting to Producer unsuccessful." << std::endl;
        }
        stream_lk->unlock();
    }
}