// From UNP

#ifndef __UNP_WRAPPER
#define __UNP_WRAPPER

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h> /* ioctl() */
/* includes for struct ifreq, etc */
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
/* networking */
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT	9877
#define	MAXLINE		4096
#define LISTENQ SOMAXCONN /* defined in linux kernel */

typedef struct sockaddr SA;

typedef unsigned char   byte;

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

int Socket(int family, int type, int protocol) {
    int n;
    if((n = socket(family, type, protocol)) < 0) {
	    perror("Error: Socket can not be created\n Reason");
	    return -1;
    }
    return n;
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr) {
    int n;
AGAIN:
    if ( (n = accept(fd, sa, salenptr)) < 0) {
#ifdef EPROTO
        if (errno == EPROTO || errno == ECONNABORTED)
#else
        if (errno == ECONNABORTED)
#endif
            goto AGAIN;
        else {
            perror("Error: Socket accept failed\n Reason");
            return -1;
        }
    }
    return n;
}

ssize_t fdgets(int socketfd, char* buffer, int size) {
	
	ssize_t received, i = 0;
	char ch = '\0';
	
	while(i < (size - 1)) {
		if (ch == '\n') break;
		received = recv(socketfd, &ch, 1, 0);
		if (received > 0) {
			if (ch == '\r') { // CR LF -> LF, CR -> LF
				received = recv(socketfd, &ch, 1, MSG_PEEK);
				if (received > 0 && ch == '\n') recv(socketfd, &ch, 1, 0);
				else ch = '\n';
			}
			buffer[i] = ch;
			i++;
		} else {
			break;
		}
	}
	
	buffer[i] = '\0';
	return i;
}

int set_nonblocking(int fd) {  
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)  
		flags = 0;  
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);  
} 

/* TUN device/interface utils */

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

#endif /* __UNP_WRAPPER */
