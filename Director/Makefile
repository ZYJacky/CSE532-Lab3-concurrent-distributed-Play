all:
	g++ -L$(ACE_ROOT)/ace -Wall -std=c++11 -pthread -DTEMPLATE_HEADERS_INCLUDE_SOURCE -o Director main.cpp Director.cpp Player.cpp Play.cpp Director_Handler.cpp -I$(ACE_ROOT) -lACE

clean:
	rm Director