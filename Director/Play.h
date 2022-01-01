/*
 *   Play.h     CSE532S Lab2 version
 *
 *   Declaration of class Play, which represents a Play script 
 *
 *   Created: 9/16/21
 *   Last edited: 11/6/21
 */

#ifndef PLAY_H
#define PLAY_H


// libraries for data structures and general utilities
#include <utility> // for pair
#include <vector>
#include <ostream> // ostream
#include <string>
#include <queue>

// libraries for concurrency design
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifndef STRUCTURED_LINE 
#define STRUCTURED_LINE
typedef std::pair<std::pair<unsigned int, std::string>, std::string> structured_line; // [[Line No., Charater Name], Line]
#endif

class Play
{   
    public:

        Play(std::vector<std::string>&, unsigned int, std::atomic<bool>* should_stop);                     // consturctor 

        void recite(std::vector<structured_line>::iterator&, std::vector<structured_line>::iterator&);

        int enter(unsigned int, unsigned int);
        int exit();

        void reset(void);
        void notify_all_cv(void);


    private:

        std::vector<std::string>& scenes;
        std::vector<std::string>::iterator scenes_it;
        std::vector<std::string>::iterator scenes_begin;

        unsigned int line_counter;   
        unsigned int scene_fragment_counter;
        unsigned int on_stage;

        bool on_stage_is_set;
        unsigned int max_on_stage;
        std::string current_speaking;

        // synchronization features
        std::mutex set_on_stage_lk;

        std::condition_variable line_cv;
        std::mutex line_counter_lk; 

        std::condition_variable frag_cv;
        std::mutex frag_counter_lk; 

        // features used to deal with misordered lines
        unsigned int pending_thread;       
        std::priority_queue<int, std::vector<int>, std::greater<int>> pq;

        std::atomic<bool>* should_stop;
};         

#endif  // define PLAY_H