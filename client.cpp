#include <fcntl.h>  /* O_RDWR */
#include <string.h> /* memset(), memcpy() */
#include <stdio.h> /* perror(), printf(), fprintf() */
#include <stdlib.h> /* exit(), malloc(), free() */
#include <unistd.h> /* read(), close() */
#include <sys/select.h> /* select() */

/* cxx */
#include <iostream>

using namespace std;

/* 3rd party libs */
#include "socketwrapper.h"
#include <tins/tins.h>

using namespace Tins;

char remote_ip[16] = ""; 

void process_arguments(int argc, char **argv) {
	int opt;
	
	while (~(opt = getopt(argc, argv, "hHs:S:"))) { 
		switch(opt) {
			case 's': case 'S':
				if (optarg) {
          strncpy(remote_ip,optarg,15);
				} else {
					fputs("?\n", stdout);
					exit(0);
				}
				break;
			case 'h': case 'H':
				fputs("?\n", stdout);
				exit(0);
				//break;
		}
  }
} 

int main(int argc, char *argv[])
{
  int socketfd, tunfd, nbytes;
  char buf[1600];

  process_arguments(argc, argv);

  // dgram
  struct sockaddr_in srvaddr;
  socketfd = Socket(AF_INET, SOCK_DGRAM/* | SOCK_NONBLOCK*/, 0);
  bzero(&srvaddr, sizeof(srvaddr));
  srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, remote_ip, &srvaddr.sin_addr);

  // tun
  tunfd = tun_open("clienttun");
  system("route add 123.123.123.123 clienttun");

  fputs("Client now running in UDP mode.\n", stdout);

  int maxfd = (socketfd > tunfd) ? socketfd : tunfd;

  while(1) {

    fd_set rd_set;
		
		FD_ZERO(&rd_set);
		FD_SET(tunfd, &rd_set); FD_SET(socketfd, &rd_set);

		int ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
		if (ret < 0 && errno == EINTR) continue;
		if (ret < 0) {
			perror("select()"); exit(1);
		}

    if(FD_ISSET(tunfd, &rd_set)) {
      nbytes = read(tunfd, buf, sizeof(buf));
      printf("Read %d bytes from clienttun\n", nbytes);
      //hex_dump(buf, nbytes);
      RawPDU p((uint8_t *)buf, nbytes);
      try {
        IP ip(p.to<IP>());
        cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;
        sendto(socketfd, buf, nbytes, 0, (sockaddr*)&srvaddr, sizeof(srvaddr));
      } catch (...) {
        continue;
      }
    }

    if(FD_ISSET(socketfd, &rd_set)) { 
      int size84 = recvfrom(socketfd, buf, 1600, 0, NULL, NULL);
      printf("Recv %d bytes from udp socket\n", nbytes);
      write(tunfd, buf, size84);
    }
  }
  return 0;
}