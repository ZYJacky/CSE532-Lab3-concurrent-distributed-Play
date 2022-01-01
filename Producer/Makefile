all:
	g++ -L$(ACE_ROOT)/ace -Wall -std=c++11 -pthread -DTEMPLATE_HEADERS_INCLUDE_SOURCE -o Producer main.cpp IO_Handler.cpp Producer.cpp Stream_Handler.cpp -I$(ACE_ROOT) -lACE

clean:
	rm Producer