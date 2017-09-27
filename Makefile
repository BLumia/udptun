CC = g++
CXXFLAGS = -g -std=c++11 -I./3rdparty/libtins/include/ -L./3rdparty/libtins/lib -ltins

all: client.elf server.elf
	@echo "Done"

client.elf: client.o socketwrapper.h
	$(CC) client.o $(CXXFLAGS) -o $@ 

server.elf: server.o socketwrapper.h
	$(CC) server.o $(CXXFLAGS) -o $@ 
	
client.o: client.cpp
	$(CC) client.cpp -c $(CXXFLAGS)

server.o: server.cpp
	$(CC) server.cpp -c $(CXXFLAGS)

clean:
	rm ./*.elf
	rm ./*.o

.PHONY: clean all
