/*
 *   Player.h    CSE532S Lab2 version
 *
 *   Declaration of class Player, which represents a player in a play script, 
 *   including its name, the lines, and related action/methods  
 *
 *   Created: 10/6/21
 *   Last edited: 11/3/21
 */

 
#ifndef PLAYER_H
#define PLAYER_H

#include "Play.h"
#include "threadsafe_queue.h"

#include <utility> // for pair
#include <vector>
#include <fstream> // for ifstream
#include <thread>
#include <atomic>

#ifndef STRUCTURED_LINE 
#define STRUCTURED_LINE
typedef std::pair<std::pair<unsigned int, std::string>, std::string> structured_line; // [[Line No., Charater Name], Line]
#endif // STRUCTURED_LINE

class Player
{
    private:
        std::vector<std::shared_ptr<Play>>& plays;
        
        threadsafe_queue<std::string>& work_queue;

        std::thread worker;  

        std::vector<structured_line> lines;    // storage of structured lines

        std::atomic<bool>* should_stop;

    public:
        Player(std::vector<std::shared_ptr<Play>>&, threadsafe_queue<std::string>&, std::atomic<bool>*);

        void enter(unsigned int play_id); // lauch worker thread
        void read(std::ifstream& in_file, std::string PlayerName); // fill structureded lines container
        void act(unsigned int play_id);
        void exit(void); 


};

#endif  // define PLAYER_H
