#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "socketwrapper.h"

/* cxx */
#include <iostream>
#include <map>

using namespace std;

/* 3rd party libs */
#include <tins/tins.h>

using namespace Tins;

typedef std::pair<IPv4Address, uint16_t> AddrPort;

int tunfd;
char buf[1600];
in_addr_t cli_v4addr, srv_v4addr;
map<AddrPort, AddrPort> snat_map;
struct sockaddr_in cliaddr, srvaddr;

void process_arguments(int argc, char **argv) {
	int opt;
	
	while (~(opt = getopt(argc, argv, "hHs:S:"))) { 
		switch(opt) {
			case 's': case 'S':
				if (optarg) {
					srv_v4addr = inet_addr(optarg);
				} else {
					fputs("?\n", stdout);
					exit(0);
				}
				fputs("?\n", stdout);
				break;
			case 'h': case 'H':
				fputs("?\n", stdout);
				exit(0);
				//break;
		}
    }
} 

void mainloop(int socketfd, SA* pcliaddr, socklen_t clilen) {
	int n, nbytes;
	socklen_t len;
	char buf[MAXLINE+1];
	
	int maxfd = (socketfd > socketfd)? socketfd : socketfd;

	for(;;) {
		fd_set rd_set;
		
		FD_ZERO(&rd_set);
		FD_SET(tunfd, &rd_set); FD_SET(socketfd, &rd_set);

		int ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
		if (ret < 0 && errno == EINTR) continue;
		if (ret < 0) {
			perror("select()"); exit(1);
		}

		if(FD_ISSET(tunfd, &rd_set)) {
			// data avaliable from tun dev
			nbytes = read(tunfd, buf, sizeof(buf));
			if (nbytes > 0) {
				printf("Read %d bytes from servertun\n", nbytes);
				//hex_dump(buf, nbytes);
				RawPDU p((uint8_t *)buf, nbytes);
				try {
					IP ip(p.to<IP>());
					cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;
					
					// dNAT
					AddrPort srcaddr, dstaddr;
					if (ip.protocol() == IPPROTO_TCP) {
						TCP& tcp = ip.rfind_pdu<TCP>();
						srcaddr = std::make_pair(ip.src_addr(), tcp.sport());
					} 
					if (ip.protocol() == IPPROTO_UDP) {
						UDP& udp = ip.rfind_pdu<UDP>();
						srcaddr = std::make_pair(ip.src_addr(), udp.sport());
					}
					dstaddr = snat_map[srcaddr];
					ip.dst_addr(dstaddr.first);

					// sent packet to client
					PDU::serialization_type serval = ip.serialize();
					memcpy(buf, serval.data(), serval.size());
					sendto(socketfd, buf, serval.size(), 0, pcliaddr, clilen);
					printf("Sent %d bytes to client\n", serval.size());
					
				} catch (...) {
					continue;
				}
			}
		}

		if(FD_ISSET(socketfd, &rd_set)) {
			// data avaliable from udp socket
			len = clilen;
			n = recvfrom(socketfd, buf, MAXLINE, 0, pcliaddr, &len);

			printf("Read %d bytes from udp socket\n", n);
			//hex_dump(buf, n);
			RawPDU p((uint8_t *)buf, n);
			try {
				IP ip(p.to<IP>());
				TCP& tcp = ip.rfind_pdu<TCP>();
				cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;

				// save source addr info
				AddrPort srcaddr, dstaddr;
				srcaddr = std::make_pair(ip.src_addr(), tcp.sport());
				dstaddr = std::make_pair(ip.dst_addr(), tcp.dport());
				snat_map[dstaddr] = srcaddr;

				// sNAT
				ip.src_addr(IPv4Address(srv_v4addr)); // src_addr = srv ip
				// no port modify now

				// write to tun device (to send packet)
				PDU::serialization_type serval = ip.serialize();
				memcpy(buf, serval.data(), serval.size());
				write(tunfd, buf, serval.size());
				printf("Sent %d bytes from servertun\n", serval.size());
				
			} catch (...) {
				continue;
			}
		}
	}
}

int main(int argc, char** argv) {

	int listenfd;
	pid_t childpid;
	socklen_t clilen;
	
	process_arguments(argc, argv);
	
	// dgram
	listenfd = Socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&srvaddr, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(SERV_PORT);
	
	bind(listenfd, (SA*) &srvaddr, sizeof(srvaddr));
	
	fputs("Server now running in UDP mode.\n", stdout);

	// server tun
	tunfd = tun_open("servertun");
	system("ip addr add 192.168.61.0/24 dev servertun");

	mainloop(listenfd, (SA *) &cliaddr, sizeof(cliaddr));
}
