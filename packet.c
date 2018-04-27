#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>

#include "main.h"
#include "man.h"
#include "host.h"
#include "net.h"
#include "packet.h"


void packet_send(struct net_port *port, struct packet *p)
{
char msg[PAYLOAD_MAX+4];
int i;

if (port->type == PIPE) {
	msg[0] = (char) p->src; 
	msg[1] = (char) p->dst;
	msg[2] = (char) p->type;
	msg[3] = (char) p->length;
	for (i=0; i<p->length; i++) {
		msg[i+4] = p->payload[i];
	}
	write(port->pipe_send_fd, msg, p->length+4);
//printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n", 
//		(int) msg[0], 
//		(int) msg[1], 
//		(int) p->src, 
//		(int) p->dst);
}
if (port->type == SOCKET) {
	//printf("Making client\n");
	struct addrinfo hints, *servinfo, *pt;
	char portName[6];
	int i;
	int yes = 1;
	int sockfd;
	int rv;
	
	char s[INET6_ADDRSTRLEN];
					
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	sprintf(portName, "%d", port->socket_send_port);
	
	//printf ("client %d: connecting to %s on port %s\n", p->src, port->domainAway, portName);
	
	if ((rv = getaddrinfo(port->domainAway, portName, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}
	
	//printf("Test: %d\n", i);
	for(pt = servinfo; pt != NULL; pt = pt->ai_next) {
		if ((sockfd = socket(pt->ai_family, pt->ai_socktype,
				pt->ai_protocol)) == -1) {
			//printf("Test:");
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, pt->ai_addr, pt->ai_addrlen) == -1) {
			close(sockfd);
			//printf("GHBJDSA");
			perror("client: connect");
			continue;
		}

		break;
	}
	
	if (pt == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return;
	}
	
	inet_ntop(pt->ai_family, get_in_addr((struct sockaddr *)pt->ai_addr),
			s, sizeof s);
	
	freeaddrinfo(servinfo); // all done with this structure
	
	memset(msg, '\0', p->length + 4);
	
	msg[0] = (char) p->src; 
	msg[1] = (char) p->dst;
	msg[2] = (char) p->type;
	msg[3] = (char) p->length;
	for (i=0; i<p->length; i++) {
		msg[i+4] = p->payload[i];
	}
	
	//test
	//msg[0] = 't';
	//msg[1] = 'e';
	//msg[2] = 's';
	//msg[3] = 't';
	//msg[4] = '\0';
	
	/*
	printf("Sending: %d%d%d%d%d%d%d%d\n", 	(int) msg[0],
											(int) msg[1],
											(int) msg[2],
											(int) msg[3],
											(int) msg[4],
											(int) msg[5],
											(int) msg[6],
											(int) msg[7]);
	
	*/
	/*
	printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n", 
		(int) msg[0], 
		(int) msg[1], 
		(int) p->src, 
		(int) p->dst);
	*/

	
	int x = send(sockfd, msg, PAYLOAD_MAX+4, 0);
	// printf("Sent %d bytes\n", x);
	
}
return;
}

int packet_recv(struct net_port *port, struct packet *p)
{
char msg[PAYLOAD_MAX+4];
int n;
int i;
	
if (port->type == PIPE || port->type == SOCKET) {
	n = read(port->pipe_recv_fd, msg, PAYLOAD_MAX+4);
	if (n>0) {
		p->src = (char) msg[0];
		p->dst = (char) msg[1];
		p->type = (char) msg[2];
		p->length = (int) msg[3];
		for (i=0; i<p->length; i++) {
			p->payload[i] = msg[i+4];
		}
/*
 printf("PACKET RECV, src=%d dst=%d p-src=%d p-dst=%d\n", 
		(int) msg[0], 
		(int) msg[1], 
		(int) p->src, 
		(int) p->dst);
*/
	}
}

return(n);
}


