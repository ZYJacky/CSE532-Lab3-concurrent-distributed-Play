CSE 532 Fall 2021 Lab 3

Jackie Zhong (jackie.z@wustl.edu)

To unpack: Run "tar -xvf Lab3_Jackie.tar.gz" 
To make: run "make" in the command line. Flags of "-Wall", "-pthread", "-std=c++11", "-DTEMPLATE_HEADERS_INCLUDE_SOURCE", and ACE related 
		 flags are already added. If more flags are needed, please modify Makefile.

--------------------------------------------

Program Overview:

		The assignment consists of two programs: "Producer" who acts like a server that accepts Director, interact with users, and sends commands to Director;
	and "Director" who acts like a client that connects to Producer, reports plays it has, and wait for commands to start a play.

		The Producer program accepts a single command line input (argc = 2) that tells it which port to listen on at the local host (127.0.0.1). Any more
	commands will be ignored; the program will warn and return if no enough commands. 
		
		The Producer first tries to bind to the given port using the "acceptor.open()", if it fails it will generate an error message and return a unique
	non-zero value. Otherwise, the Producer gets a reactor instance and tries to register three handlers whose class are all inherited from ACE_Event_Handler:
	
		- IO_Handler: the IO_Handler is responsible for std::cin event. It uses the ACE_STDIN handle, handle_input() method, and is registered with a READ_MASK.
	After receiving and processing a std::cin input using std::getline(), it passes the argument to the Producer class instance. The producer has a method that determine
	if the command is to "start", "stop", or "quit". If the former 2, the method checks the status of the Play and sends according message through the stream.
	If the last, it will perform as if a Ctrl+C is received (see below).
		
		- Connection handler: the Producer class is also registered with ACCEPT_MASK, has a handle from the acceptor, and overrides the virtual handle_input method.
	If the "acceptor.accept(stream)" method succeeds (otherwise print a error message), the handler will construct dynamically a Stream_Handler() object and register 
	it with the reactor. The stream_handler class is for receiving message, it has a handle from the connected stream and overrieds the handle_input method. 
	The stream_handler is an abstraction for the connected Director class. It is responsible for receiving message reactively from the Director, which may send
	a message indicating that it has go offline, or a Play has finished. In either case, the stream_handler will keep track of the status of its corresponding
	Director, and will unregister and delete itself if its Director goes offline. The Producer has a list of stream_handler that it will keep track of, and 
	assigning new incoming Director a id corresponding to its location in the list.
	
		- SIGINT: the Producer class has a handle_signal() method and will handle Ctrl+C. The handle method ends the program by calling reacot->end_event_loop()
	and reactor_close(). The end_event_loop() method will invoke unregister handler for the stream_handlers that are stored in Producer. The handle_close method
	will thereby get invoked and close the socket stream to let its director knows that the Producer has gone offline. Then the method will return and since the
	event loop has ended, the program returns gracefully.

		Finally, the Producer starts the reactor's event loop. See below for an illustration of the Producer's structure. The Producer works in a completely
	reactive manner.
		
								   Reactor
						----------------------------------------------
					
		std::cin ---->				          IO_Handler                         
                                                                                                 

      Connection request --->					   Producer
				     				/     |      \  
                                    			       /      |       \
                                    		              /       |        \
	Message from Director ------>	       	Stream_Handler  Stream_Handler  Stream_Handler
                                                       |             |                |
 						       |             |                | 
						       |             |                |
                                                    Director0     Director1	   Director2



		The Director progam takes at least four arguments: two for address and port_num of server to connect, one for minimum number of player to construct,
	and all the rest will be regarded as names of play scripts. If less arguments are provided, the program prints usage message and return.
		
		Director will check and make sure each provided arguments is expected (e.g. port_num is indeed a number), and make sure the script files can be open.
	It then attempts to connect to the specified address using "connector.connect()". If that fails it will print error message and return unique non-zero message.
	
		If the connection is successful, it will construct a Director() class instance. The constructor of director takes the name of all valid script files.
	It will iterate through each of them to process the definition of character parts, count maximum number of players in two consecutive scenes, etc. Finally,
	it will construct that many plays and the max of the max consecutive players and the provided minimum number of players and construct that many player objects.

		Then, the director constructs an ACE_Reactor and a Director_Handler() object. The Director_Handler object is registered with the reactor as below:

		- The director handler object has a handle from the connected stream and an overriden handle_input() method. It is registered with READ_MASK to
	reactively process message from the stream_handler. If the message is a single number, the director will call cue() on the corresponding play (see below).
	If the message is stop, it will terminate the ongoing play by setting a stop flag (see desgin choice section). 

		- The director_handler object also has a handle_signal method and is registered with reactor with SIG_INT. Upon receiving a Ctrl_C, the Director
	will tell its play to stop by setting a flag and joining with the thread that recites play. Then cleanup resources (e.g. stream->close, end_eventlopp(), etc.) 

		If registration are successful (complain and end if not), the director sends a message over the stream to tells the Producer its play list, and start 
	the event loop.		

		The design of Play, Player, and cue() is almost exactly the same from Lab2 (see below) with a few minor changes: the cue method now takes a parameter
	of which play to perform, which will cause the Play and Player to look for scene and part definitions at the corresponding slot in the container; there is 
	a should_stop flag of type atomic<bool>	in the Director and passed to Play and Players, who will check reasonably often (see design choices); finally,
	after each cue() run, the Director will reset the status of that play (e.g. line counter, scene_frag counter, etc.), and send a message across the stream
	to let the Producer know.
			
		
	Cue, Play, and Player:

		 The cue() method implements a HS/HA pattern. The synchronous part are the players which are active objects that have internal thread. These players 
	will keep pulling on the job queue, get a player definition, and recite it. The asynchronous part is the handing off of player job into the queue. See the 
	picture below.


			Player pools                                Director
			------------				   ----------
			pull from queue      			    put jobs 	
			process job:				    into queue
			{
			  read()           <--  Queue of jobs <--   
		          enter()
			  recite()
			}
	
		The cue() methods call each player's enter() method, which will lauch the internal thread of the active player object. The thread runs the act()
	method that repeatedly pop from the queue in a thread_safe manner. The cue() methods then push all jobs into the queue in a thread_safe manner, and then
	push a token signifying the end of all jobs. It finally calls exit() on all player that will wait and join with joinable player thread.

		When the player thread gets something from the queue, if checks if the thing is an end token, if so it break out of the loop and will be joined
	by the exit() method if joinable. Otherwise, it process the job. A job specifies four things: the scene number, the valid players in this scene, 
	the character name, and the name of the character file (which is checked to be valid previously). It first read() from the file that populates a container
	of structured lines consist of line number, role name, and actual line. Then it call the enter() method of the play passing it the scene number and the
	valid players in this scene.

		The Play class keeps an internal counter of scene. When a player tries to enter the play, the scene number will be compared with the scene counter,
	and will be blocked (if bigger) and complained (if smaller) if it is not the right scene. In other words, the enter() method serves as a barrier that 
	only let go the player that are in current scene. The first player that enter the play will let it know how many players are in this scene.

		The Player then repeatedly recite() the lines to the play. The implementation of this part is almost identical to that in the previous lab in the 
	following manner:

		1. A unique_lock<std::mutex> guards the entire scope of recite(). Player thread that holds the mutex checks the line number against a counter that the
	play object holds, which indicates the next line that it is expecting to recite (initialized to 1).

		2. If the couter is less than the line number, that means it is not the play's term yet. It first pushes its line number to a min-first priority queue
	that keeps track of the line numbers of currently pending thread. Then it increments a pending_thread variable of the Play object, and check to see if the 
	pending_thread equals the current acting players (a variable of Play object that is set at the construction initialized to valid_role_count).
	If so, that means the line numbers are misordered and it will set the counter to line number at top of the priority queue to make sure the program still proceeds.
	It itself is the thread that should proceed, it will pop its line number from the queue, decrement the pending thread, and proceeds to step 4. Else it will notify
	all the pending thread that counter has changed, and go to sleep. When it wakes up it will also do the same pop and decrement operation.

		3. If the counter is bigger than the line number, that means the lines are out of order. Since the program aims to always recite the line in ascending
	order, this line will be skipped by incrementing the player's iterator. Before it exits, it will check to see if this is the last line of this player (by checking
	if iterator equals containter.end() which is passed as a variable), if so and the rest of the active threads are all pending, it has to update the counter to the
	top of the priority_queue to avoid livelock and also update the number of acting role to signify it finishes. It then notifies all thread and simply return.

		4. If the counter is equal to the line number, the passed structured line will be printed out. If the player that speaks is different from the previous one
	that speak (which the Play object keeps track using a variable). It also makes a blank line and prints the new Player's name. Again, if the current player is finished,
	to avoid potential livelock in a misordered scenario, it will check to see if all the rest of the acting roles are waiting, if so it updates the counter to the 
	top of the priority queue. Otherwise it simply increments the counter. It then notifies all and return.

		The one difference here is when the player has recite all its lines, before it goes back and try to grab a new job, it will call exit() method. If this
	player is not the last player to exit this scene, it will simply decrement the number of players in this scene, check if all the rest are blocked (in an unordered)
	scenario and update the line counter accordingly, then leave. If it is the last player to exit, it has to do extra clean up, including resetting the line counter,
	print name of the next scene, increments the play's scene counter, and notify all the players that are blocked at the barrier.

		Finally, when the end token has been seen, the thread will return and joined by director.
	
		
----------------------------------------------

Wrapper Facades Pattern:

	thread():

		This wrapper provided by the <thread> library combines low-level function (pthread_create(), pthread_exit(), etc.) into a thread object. This simplifies
	the management of threads from calling different pthread functions to playing with a concrete object which is more maintainable and hides many low-level details
	from the program.
	

	Director class and its member functions:

		The director class is responsible for constructing the Play and Players, as well as the asynchronous part of handing out the job in the HS/HA pattern.
	By wrapping all these features in the Director class, it explicitly does the asynchronous part, and do not worry about anything in the synchronous realm, which
	increases the modularity and separation of function.


	Player class and its member functions: 

		The Player class are active objects that does the synchronous part of the pattern by repeatedly getting job from queue and processing it. Again, it
	does not worry about the other end, which is another good example of modularity and separation of function in Wrapper Facades.

	
	Play class and its member function:
	
		The Play class is responsible for the detailed synchronization between scenes and lines. Separating it from the Player class avoid having a extraordinary
	amount of member variables (especially those for synchronization) and related functions, making the program easier to be thought of and increasing its modularity.
	

	Director Handler:

		The director handler takes all functions related with managing communication with the Producer together, thus making other existing functionality in
	Director, Player, and Play completely separate. In fact, there are very little modifications to those parts from lab 2. This increases the program's modularity
	and is easy to debug.
  

	ACE Library:
	
		The ACE library provided some very helpful instances that wrap related functions together ,hides low level details, and are easy to use with good interface.
	 Specifically:

			- The acceptor, connector, and ACE_stream methods hides low level detail of socket connection.

			- The reactor instance hides detail of polling/selecting and provides an easy to use reactive interface
			  by registering handler that it dispactches serially. 	


	Producer, stream handler, and IO Handlers

		These three handler classes separates the functionality of handling connection, handing user input, and handling client's messages. They do not interfere
	with each others by wrapping there own related functions and data together.	
		
	  		
-----------------------------------------------

Design Choices and observations:

	Storage of play line:

		As required, a play line consist of the character name, the line number, and the actual line. To store this in a container in the Play object, a pair of
	a pair and a string is used and declared as follow:

		typedef std::pair<std::pair<unsigned int, std::string>, std::string> structured_line; // [[Line No., Charater Name], Line]

	By doing so, the program uses standard data structure and avoids designing a custom one, which means it can simply uses the standard library
	funtion when necessary to simplify the operation.  
	
	HS/HA pattern:

		The HS/HA pattern is briefly explained in the overview. Here is a little bit more details:

		The job queue that is used for communication between the synchronous and asynchronous part is a threadsafe_queue() inspired by the threadsafe_queue
	in Anthony Williams' C++ Concurrency in Action, Second Addition introduced in chapter six. It basically ensures thread safe by having an internal lock,
	and use condition variable whenever something is pushed to notify that may be waiting for the data, in this case are the player threads that are waiting
	for job from the job queue. The job has a specific format that are explained in the overview section.
	
	
	Concurrency design:

		This lab has two major concurrency part, separating by the barrier at entry point. The barrier will block all players that are not in current scene,
	which means a thread is in either one of the concurrency state: either it is entering and being blocked, or it is reciting its part. This two concurrency parts
	does not interfere with each other.

		The condition that separates this two parts is the scene_fragment_counter of the play class. It is only used when entering and exiting and therefore
	does not interfere with the reciting synchronization within a scene. A specific lock and condition variable is used for this variable-related operations.

		The other part is all the players within a scene. They will tries to recite their lines. This part is pretty much the same as the previous lab with
	a few small changes, see the Program overview for details.

		In addition to the concurrency within each part (e.g. blocking on condition varaible to let others proceed), the entering and the reciting are
	almost completely concurrent and parallel and do not interfere. The only small exception is when the last player of a scene finishes reciting and is 
	exiting, it needs to temporarily use the lock for fragment counter, making it race for the lock with the entering a bit.
	

	Managing connections:

		The producer generates one instance of Stream_Handler() class for every incoming Director connection. A director_id is given corresponding
	to the location of the instance in Producer's container, so that each Director can be identified and when Producer receives the message from
	Director it knows which director it comes from as the corresponding Stream_Handler will take care. Another benefit of doing so is each stream_handler
	keeps track of its associated Director's status, including play list, which play is ongoing, and etc., so it becomes easier to use these resources.
	Finally, the Producer also keeps another container that is of the same length of that storing stream_handlers. This container tells wheter
	or not a Director is still connected, so that the director_id of disconnected stream can be reused when new connection comes in.
	
	on_stage:

		Instead of incrementing a variable whenever a player enters and decrementing it whenever it leaves, since we are provided with the full config. file	
	at the beginning, the program instead just count how many valid players should be on_stage in a scene, and only decrement that when one leaves.

		The reason for doing so is using a static variable has less synchronizing issues than a dynamic one, consider the following case:

		1. Player A in a scene enter, incrementing the on_stage
		2. A finishes reciting and exit, decrementing the variable. Since it is the only player then, the scene_fragment_counter increment
		3. Player B from the same scene of A enter, finding that it is in the wrong scene and has to abort.

		Therefore, to make sure the program has a more consistent output that let all valid players speak, the on_stage variable is pre-determined by valid
	palyer count in a scene.

	
	Reactive producer:
		
		The producer needs to handle connection, input over socket, input over stdin, and SIGINT. One thing to note is all of these events takes a short
	time to process and they do not block thanks to the ACE_Reactor handler polling. Therefore, it seems to be a reasonable approach to do a complete reactive 
	pattern with ACE_Reactor, since there isn't much concurrency to extract. In addition, a completely reactive pattern also saves the implementation and 
	overheads associated with threading.

	User input:

		When the user asks to start a play, the reactor invokes the handle_input method of IO_Handler, which use std::getline() to extract the command
	and invoke the Producer's process_command() method. 

		- If the input is start a play, the method will examine the provided arguments, and will complain if the arguments is not in the right format.
		  Else, it will invoke the corresponding Stream_Handler's ask_for_play() method, which sends a message over the stream. Once the director receives
	          the message, it will try to lauch another thread calling director.cue(play_num) to recite. This is because we need the functionality to stop
		  a play, and if this is not done in a thread, the stop will never happens before the play finishes due to the serial dispatching of the reactor.
		  The director will then sends a message back to the Producer to indicates the play has started, who will update and display the status of the 
		  play list accordingly.

		- If the input is to stop a play, the method will examine the provided arguments, and will complain if the arguments is not in the right format.
		  Else, it will invoke the corresponding Stream_Handler's stop_a_play() method, which sends a meessage over the stream. Once the director
		  receives the message, it will set a flag in director that Play and Players can see. This flag is being checked at the start of Player's
		  enter and recite method, after each condition variable wait, and after each recite in player.act(). The reasoning is as follow: when a 
		  flag is set, the thread is either reciting, or being blocked (since it is not its scene or line yet). Either way, after it finishes reciting
		  or if it gets unblocked, it will see that flag. Player that sees that flag will break from its work loop immediately, before which 
		  it will notify_all() the condition variable in case there are still threads blocked (flag is checked in condition variable as well). The 
		  main thread that receives the stop message will join with the reciting thread (cue()), which join with each players, reset the play state
	          (line counter, etc.), and send a message to the Producer which can refresh the play list.

		- If the input is "quit", the Producer will behave as if Ctrl_C is pressed.See below.

	SIGINT:

		When Ctrl-C is pressed in the Director, its handler will receive the signal, asks all play to stop (by setting a flag) and join with the 
	reciting thread (who may or may not be active at this point). Then, it will closes the stream, which will invoke the handle_input() method of the
	Producer who gets a return value =0 for recv(stream), thereby knowing the Director has left, and will remove the handler of the Stream_Handler
	associated with this director (which will delete itself and the stream in handle_close) print a message indicating that and update
	the Play list. The director then calls end_event_loop() and close on reactor to clean up, and will end gracefully.

		When Ctrl-C is pressed int the Producer, its handler will receive the signal and calls end_event_loop() and close on reactor. For any 
	streams that are still connected with the Producer, since the closing of reactor will causes all handlers to be unregistered, the handle_close()
	methods of the Stream_Handler() whose associated Directors are still active will be invoked. The handle_close method will close down the stream,
	which will invoke the handle_input() method of the Director who gets a return value =0 for recv(stream), thereby knowing the Director has left and
	will clean up and end gracefully. Note that unlike start and stop, the Producer will not wait for confirmation from the Director. The rationale
	is if for some reason the Director does not receive the message and does not terminate, the confirmation will never get back to the Producer 
	(even if it does it is meaningless if Director does not end gracefully), and the Producer will be blocked and can't end due to not receiving the 
	confirmation. 
	
		

------------------------------------------------

Operation Instructions:

	To unpack: Run "tar -xvf Lab3_Jackie.tar.gz" 

	To make: run "make" in the command line. Flags of "-Wall", "-pthread", "-std=c++11", "-DTEMPLATE_HEADERS_INCLUDE_SOURCE", and ACE related 
		 flags are already added. If more flags are needed, please modify Makefile.

	To run: to run the Producer/server, issue:
		
			./Producer <port>

		to run Director/client, issue:
			
			./Director <port> <ip_address> <min_threads> <script_file>+

		To start a play from producer: 
			
			start <Director id> <play id>

		To stop a play:
	
			stop <Director id> <play id>

		To quit:
		
			quit 

			      
	
------------------------------------------------

Evaluations:

	The program is being tested with the following test cases:

	1. Running two Producer on the same port. The later one returns stating cannot bind to that port as expected.

	2. Running Director with a script file name that does not exist. The director complains and returns as expected.

	3. Running Director with one empty script file. The director take that file and the producer can ask it to start, but nothing gets printed out as expected.

	4. Running one Director and Producer. The Producer can ask the Director to start any file, and it updates the status of list of play correctly. The director		 
	   correctly recites the file upon request. And after finishing one file the Producer can ask the director to start the same of another play without any
	   problem.

	5. Running multiple directors (I tried 4 at most) with one producer. The Producer keeps track of the list of play from each director correctly. And when
	   a director leave and rejoin, the producer correctly assign it with the smallest available director id. The Producer can ask any of the director to
	   start any play. And when the director leaves, it correctly updates the list. When the producer ends, all director detects that and end gracefully.

	6. Running invalid commands in the Producer, such as starting a play that does not exist, stopping a play that is not in progress, etc. The Producer 
	   complains about that and do nothing as expected. 

	7. Testing the stop feature by adding a std::cin.ignore() at where the Player thread recites each play to give enough time to enter the stop command. 
	   As expected, the after giving the stop command the play will end. And when terminating the Producer or director when the play is in progress,
	   both the producer and all director reacts as expected. And after ending a play, the director still lives and can continue to receive request
	   to recite its Play as expected.


-------------------------------------------------

Extra Credit Section:

		The proctocol that will tolerate missing and duplicated lines are already implemented from the previous labs. This lab only add some extra lines to 
	incorporate this feature but fundamental idea is still the same. 

		In an ordered case, each player thread will either wait for its term, or one thread is guaranteed to have the counter equal its line number and can 
	therefore increment the counter and notify all other threads to proceed. In an misordered case, however, the situation is much more complicated. Consider the 
 	following case:

		1. The counter defaults to start at 1.
		2. All players' script is misordered, and none of their first line start with 1.
		3. They all block by the condition variable wait(), and the program livelock.

		This scenario can not only happen at the beginning, but can also happen in the middle. To deal with this potential livelock, one solution is to have a
	monitor thread checks at a time interval how many threads are blocked (by letting thread increment a variable before they wait()) and perform action is all
	threads are blocked. The issue with this is it is hard to tune the time interval and synchronize the monitor thread with the rest of the threads. Instead,
	this design let the player thread themselves to get out of the livelock situation. All threads when entering the wait phase (counter < line number) will push its
	line number onto a min-first priority queue, so that in the misordered scenario described just now, we now which line we should jump to. If a thread sees that
	all threads including itself is in the wait phase (by checking a variable that each thread increments and decrements entering and leaving the phase), it will
	change the counter to directly jump to the smallest current line (top of min heap), and then notify_all to make sure the program proceeds.

	
	Another case is the counter is larger than the line number, per the requirement of the lab to at least print out in ascending order, this case is handled by simply 
	print out a warning and skip it by incrementing the counter. However, this is an edge case such that if this is the last line of the player, if the thread simply
	prints out the warning and exit, if all the rest of threads are blocked, the program livelock again. Again, to avoid this scenario we have to make sure the program
	increases the counter accordingly in every possible path. Therefore additional condition checking is done before the thread exit the recite method in the bigger case (counter > line number)
	and equal case (counter == line number) to see if this is the last line of the player. If so and all the rest of the threads that haven't finished acting are all blocked,
	need to again let counter be the top element on the priority queue to make sure the program proceeds. Notice that in an ordered scenario the top of the priority queue
	is always going to be the next line number as it is min-first. 

	This methodology will generate a consistent output (for part of the lines that are in order) for misorded lines and is guarenteed to maximize
	the ascending lines, which means it will start comparing the first line of each character, take the smallest, move the line iterator for that
	character, and repeatedlly perform this opeartion (with min-first heap) to assure that maximum ascending sequnce across the files are achieved.

	The example output for misordered input part files are as followed. Duplicated and misordered lines are reported, missing lines are ignored.

			King.
			Thanks, Rosencrantz and gentle Guildenstern.
			---warning: line 1 of King out of order
			---warning: line 2 of King out of order
			---warning: line 3 of King out of order
			---warning: line 4 of King out of order
			---warning: line 5 of King out of order
			---warning: line 6 of King out of order
			---warning: line 7 of King out of order
			---warning: line 7 of King out of order


	           






	