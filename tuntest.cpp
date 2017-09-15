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

/* cxx */
#include <iostream>

using namespace std;

/* 3rd party libs */
#include <tins/tins.h>

using namespace Tins;

typedef unsigned char   byte;

int tun_open(const char *devname)
{
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
/*
bool doo(PDU &some_pdu) {
  // Search for it. If there is no IP PDU in the packet, 
  // the loop goes on
  IP &ip = some_pdu.rfind_pdu<IP>(); // non-const works as well
  cout << "Destination address: " << ip.dst_addr() << std::endl;
  PacketSender sender("dummytun");
  auto tmp_src = ip.src_addr();
  auto tmp_dst = ip.dst_addr();
  ip.src_addr(tmp_dst);
  ip.dst_addr(tmp_src);
  sender.send(some_pdu);
  return true;
}
*/
int main(int argc, char *argv[])
{
  int fd, nbytes;
  char buf[1600];

  fd = tun_open("dummytun");
  //system("ifconfig dummytun up");
  system("route add 123.123.123.123 dummytun");
  //Sniffer sniffer("dummytun");
  //sniffer.sniff_loop(doo);

  while(1) {
    nbytes = read(fd, buf, sizeof(buf));
    printf("Read %d bytes from dummytun\n", nbytes);
    //hex_dump(buf, nbytes);
    RawPDU p((uint8_t *)buf, nbytes);
    try {
      IP ip(p.to<IP>());
      cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;
      Tins::IPv4Address srcaddr = ip.src_addr();
      ip.src_addr(ip.dst_addr());
      ip.dst_addr(srcaddr);
      ICMP &icmp = ip.rfind_pdu<ICMP>();
      icmp.type(ICMP::ECHO_REPLY);
      write(fd, ip.serialize().data(),ip.serialize().size());
    } catch (...) {
      continue;
    }


    // IP *ip = p.find_pdu<IP>();
    // if (ip != NULL) {
    //   cout << "Destination address: " << ip->dst_addr() << std::endl;
    // }
    //IP *IpPDU = new IP((uint8_t *)buf, nbytes);
    //cout << "Destination address: " << IpPDU->dst_addr() << std::endl;
  }
  return 0;
}