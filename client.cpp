#include <fcntl.h>  /* O_RDWR */
#include <string.h> /* memset(), memcpy() */
#include <stdio.h> /* perror(), printf(), fprintf() */
#include <stdlib.h> /* exit(), malloc(), free() */
#include <sys/ioctl.h> /* ioctl() */
#include <unistd.h> /* read(), close() */

/* includes for struct ifreq, etc */
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
/* networking */
#include <netinet/in.h>
#include <arpa/inet.h>

/* cxx */
#include <iostream>

using namespace std;

/* 3rd party libs */
#include "socketwrapper.h"
#include <tins/tins.h>

using namespace Tins;

typedef unsigned char   byte;

int tun_open(const char *devname) {
  struct ifreq ifr;
  int fd, err;

  if ( (fd = open("/dev/net/tun", O_RDWR)) == -1 ) {
        perror("open /dev/net/tun");exit(1);
  }
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI | IFF_UP | IFF_RUNNING;
  strncpy(ifr.ifr_name, devname, IFNAMSIZ);  

  /* ioctl will use if_name as the name of TUN 
    * interface to open: "tun0", etc. */
  if ( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) == -1 ) {
    perror("ioctl TUNSETIFF");close(fd);exit(1);
  }

  if ( (err = ioctl(socket(PF_INET, SOCK_DGRAM, 0), SIOCSIFFLAGS, (void *) &ifr)) == -1 ) {
    perror("ioctl SIOCSIFFLAGS");close(fd);exit(1);
  }

  /* After the ioctl call the fd is "connected" to tun device specified
    * by devname */
  printf("Device %s opened\n", ifr.ifr_name);

  return fd;
}

void hex_dump(const char *buf, int len) {
	const char* addr = buf;
    int i,j,k;
    char binstr[80];
 
    for (i=0;i<len;i++) {
        if (0==(i%16)) {
            sprintf(binstr,"%08x -",i+addr);
            sprintf(binstr,"%s %02x",binstr,(byte)buf[i]);
        } else if (15==(i%16)) {
            sprintf(binstr,"%s %02x",binstr,(byte)buf[i]);
            sprintf(binstr,"%s  ",binstr);
            for (j=i-15;j<=i;j++) {
                sprintf(binstr,"%s%c",binstr,('!'<buf[j]&&buf[j]<='~')?buf[j]:'.');
            }
            printf("%s\n",binstr);
        } else {
            sprintf(binstr,"%s %02x",binstr,(byte)buf[i]);
        }
    }
    if (0!=(i%16)) {
        k=16-(i%16);
        for (j=0;j<k;j++) {
            sprintf(binstr,"%s   ",binstr);
        }
        sprintf(binstr,"%s  ",binstr);
        k=16-k;
        for (j=i-k;j<i;j++) {
            sprintf(binstr,"%s%c",binstr,('!'<buf[j]&&buf[j]<='~')?buf[j]:'.');
        }
        printf("%s\n",binstr);
    }
}

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
  tunfd = tun_open("dummytun");
  //system("ifconfig dummytun up");
  sendto(socketfd, "startfuck", 9, 0, (sockaddr*)&srvaddr, sizeof(srvaddr));
  system("route add 123.123.123.123 dummytun");

  while(1) {
    nbytes = read(tunfd, buf, sizeof(buf));
    printf("Read %d bytes from dummytun\n", nbytes);
    //hex_dump(buf, nbytes);
    RawPDU p((uint8_t *)buf, nbytes);
    try {
      IP ip(p.to<IP>());
      cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;
      sendto(socketfd, buf, nbytes, 0, (sockaddr*)&srvaddr, sizeof(srvaddr));
      sendto(socketfd, "fuck", 4, 0, (sockaddr*)&srvaddr, sizeof(srvaddr));
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