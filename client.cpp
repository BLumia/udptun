#include <fcntl.h>  /* O_RDWR */
#include <string.h> /* memset(), memcpy() */
#include <stdio.h> /* perror(), printf(), fprintf() */
#include <stdlib.h> /* exit(), malloc(), free() */
#include <unistd.h> /* read(), close() */

/* cxx */
#include <iostream>

using namespace std;

/* 3rd party libs */
#include "socketwrapper.h"
#include <tins/tins.h>

using namespace Tins;

int main(int argc, char *argv[])
{
  int socketfd, tunfd, nbytes;
  char buf[1600];

  // dgram
  struct sockaddr_in srvaddr;
  socketfd = Socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&srvaddr, sizeof(srvaddr));
  srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &srvaddr.sin_addr);

  // tun
  tunfd = tun_open("clienttun");
  system("route add 123.123.123.123 clienttun");

  while(1) {
    nbytes = read(tunfd, buf, sizeof(buf));
    printf("Read %d bytes from clienttun\n", nbytes);
    //hex_dump(buf, nbytes);
    RawPDU p((uint8_t *)buf, nbytes);
    try {
      IP ip(p.to<IP>());
      cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;
      sendto(socketfd, buf, nbytes, 0, (sockaddr*)&srvaddr, sizeof(srvaddr));
      int size84 = recvfrom(socketfd, buf, 1600, 0, NULL, NULL);
      if (size84 == 84) {
        write(tunfd, buf, size84);
      }
    } catch (...) {
      continue;
    }
  }
  return 0;
}