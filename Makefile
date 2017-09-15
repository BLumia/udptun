CC = g++
CXXFLAGS = -g -std=c++11 -I./3rdparty/libtins/include/ -L./3rdparty/libtins/lib -ltins -lpcap

all: fuck.elf
	@echo "Done"

fuck.elf: tuntest.o
	$(CC) tuntest.o $(CXXFLAGS) -o $@ 
	
tuntest.o: tuntest.cpp
	$(CC) tuntest.cpp -c $(CXXFLAGS)

clean:
	rm ./*.elf

.PHONY: clean all
