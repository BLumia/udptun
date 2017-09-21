// From UNP

#ifndef __UNP_WRAPPER
#define __UNP_WRAPPER

#include <errno.h>
#include <fcntl.h>

#define SERV_PORT	9877
#define	MAXLINE		4096
#define LISTENQ SOMAXCONN /* defined in linux kernel */

typedef struct sockaddr SA;

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

#endif /* __UNP_WRAPPER */
