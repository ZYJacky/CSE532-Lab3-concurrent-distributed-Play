/*
 *   Play.cpp    CSE532S Lab2 version
 *   
 *   Implement member functions declared in Play.h
 *
 *   Created: 9/16/21
 *   Last edited: 11/1/21
 */

#include "Play.h"
#include "utility.h"

#include <iostream> // for cout, endl

// notify all condition variable
void Play::notify_all_cv(void)
{   
    line_counter_lk.lock();
    line_cv.notify_all();
    line_counter_lk.unlock();

    frag_counter_lk.lock();
    frag_cv.notify_all();
    frag_counter_lk.unlock();
}

// reset variables that indicates play status
void Play::reset(void)
{
    line_counter = 1;
    scene_fragment_counter = 1;
    scenes_it = scenes_begin;
    on_stage_is_set = false;
    current_speaking = "";
    pending_thread = 0;

    while(!pq.empty()) pq.pop();
}

// construct the Play object based off the list of scenes
Play::Play(std::vector<std::string>& _scenes, unsigned int _max_on_stage, std::atomic<bool>* _should_stop)
            : scenes(_scenes), scenes_it(_scenes.begin()), scenes_begin(_scenes.begin()), line_counter(1), scene_fragment_counter(1), on_stage_is_set(false), max_on_stage(_max_on_stage), should_stop(_should_stop)
{} 

// a barrier that only let go those players that are in the current scene (indicated by scene_fragment_counter)
int Play::enter(unsigned int frag_num, unsigned int role_count)
{      
    // this barrier will block all threads not ready to recite (not its scene yet)
    std::unique_lock<std::mutex> lk(frag_counter_lk);
    if(*should_stop) return SUCCESS;
    
    if(frag_num > scene_fragment_counter)
    {   
        frag_cv.wait(lk, [this, frag_num]{return (frag_num == scene_fragment_counter) || (*should_stop);});
        if(*should_stop) return SUCCESS;
    }
    else if(frag_num < scene_fragment_counter)
    {
        std::cerr << "warning: wired scene_fragment_counter" << std::endl;
        return WIRED_FRAG_COUNT;
    }

    // the first thread that grabs this lock set the on_stage
    set_on_stage_lk.lock();
    if(!on_stage_is_set)
    {   
        // prints current scene
        if(scenes_it != scenes.end())
        {   
            std::cout << std::endl << std::endl << *scenes_it << std::endl;
            ++scenes_it;
        }
        on_stage_is_set = true;
        on_stage = role_count;
    }
    set_on_stage_lk.unlock();

    return SUCCESS;
}

// exit a player and update counters accordingly
int Play::exit()
{   
    if(on_stage > 1)
    {   
        --on_stage;

        // in an unordered scenario, to avoid livelock if the current role finished all his
        // lines, it still may have to update the counter if other threads are in wait()
        // to avoid livelock
        if(pending_thread == on_stage)  line_counter = pq.top(); // all the rest of the players are blocked
        else ++line_counter;

        line_cv.notify_all();
    }
    else if(on_stage < 1)
    {
        return WIRED_ON_STAGE;
    }
    else if(on_stage == 1)   // last player in this scene
    {   
        // reset variables for the next scene
        on_stage_is_set = false; 
        line_counter = 1;
        current_speaking = "";

        // let those that are blocked in enter() proceed
        std::unique_lock<std::mutex> lk(frag_counter_lk);
        ++scene_fragment_counter;
        frag_cv.notify_all(); 
    }
    return SUCCESS;
}

/* Takes a reference to the player's line, check the counter, and block if it is not its turn yet
 * using wait(). Otherwise print out the line Additional features that deal with unordered player lines are implemented. See
 * lines are implemented. See ReadMe.txt for details.
 */
void Play::recite(std::vector<structured_line>::iterator& it, std::vector<structured_line>::iterator& end)
{   
    std::unique_lock<std::mutex> lk(line_counter_lk);

    if(*should_stop) return;

    unsigned int lineNo = (*it).first.first;

    if(line_counter < lineNo)
    {   
        pq.push(lineNo);
        
        ++pending_thread;

        // all other threads are waiting, need to update counter to avoid livelock
        // in the case the the threads allowed are less than the players, need 
        // to instead wait on the threads allowed
        if(pending_thread == std::min(on_stage, max_on_stage)) line_counter = pq.top();

        // this is not the thread that should proceed 
        if(line_counter != lineNo)
        {   
            line_cv.notify_all();
            line_cv.wait(lk, [this, lineNo]{return (line_counter == lineNo) || (*should_stop);});
            if(*should_stop) return;
        }
        
        pq.pop();
        --pending_thread;
    }
    else if(line_counter > lineNo)
    {   
        std::cerr << "---warning: line " << lineNo << " of " << (*it).first.second << " out of order" << std::endl;
        ++it;

        // if finished its part, need to let others know and potentially do cleanup work
        if(it == end)
        {
            int ret_val = exit();
            if(ret_val != SUCCESS)
            std::cerr << "warning: wired on_stage" << std::endl;
        }
        return;
    }

    // when counter == current line number
    if((*it).first.second != current_speaking)  // switch speaking character
    {
        current_speaking = (*it).first.second;
        std::cout << std::endl << current_speaking << "." << std::endl;
    }
    std::cout << (*it).second << std::endl;
    ++it;

    // if finished its part, need to let others know and potentially do cleanup work
    if(it == end)
    {
        int ret_val = exit();
        if(ret_val != SUCCESS)
        std::cerr << "warning: wired on_stage" << std::endl;
    }
    else 
    {
        ++line_counter;
        line_cv.notify_all();
    }
}




