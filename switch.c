 /*
  * switch.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "host.h"
#include "switch.h"
#include "packet.h"

#define MAX_FILE_BUFFER 1000
#define MAX_MSG_LENGTH 100
#define MAX_DIR_NAME 100
#define MAX_FILE_NAME 100
#define PKT_PAYLOAD_MAX 100
#define TENMILLISEC 10000   /* 10 millisecond sleep */
#define MAX_FWD_LENGTH 100

/*
 *  Main
 */

void switch_main(int switch_id)
{

/* State */
char dir[MAX_DIR_NAME];
int dir_valid = 0;

char man_msg[MAN_MSG_LENGTH];
char man_reply_msg[MAN_MSG_LENGTH];
char man_cmd;

struct net_port *node_port_list;
struct net_port **node_port;  // Array of pointers to node ports
int node_port_num;            // Number of node ports

int ping_reply_received;

int i, k, n;
int dst;
char name[MAX_FILE_NAME];
char string[PKT_PAYLOAD_MAX+1];

FILE *fp;

struct packet *in_packet; /* Incoming packet */
struct packet *new_packet;

struct net_port *p;
struct host_job *new_job;
struct host_job *new_job2;

struct job_queue job_q;

struct switch_fwd fwd_table[MAX_FWD_LENGTH];

for (int i=0; i<MAX_FWD_LENGTH; i++)
	fwd_table[i].valid = 0;

node_port_list = net_get_port_list(switch_id);

	/*  Count the number of network link ports */
node_port_num = 0;
for (p=node_port_list; p!=NULL; p=p->next) {
	node_port_num++;
}
	/* Create memory space for the array */
node_port = (struct net_port **)
	malloc(node_port_num*sizeof(struct net_port *));

	/* Load ports into the array */
p = node_port_list;
for (k = 0; k < node_port_num; k++) {
	node_port[k] = p;
	p = p->next;
}

/* Initialize the job queue */
job_q_init(&job_q);

int flag;

while(1) {

	flag = 0;

	for (k = 0; k < node_port_num; k++) { /* Scan all ports */

		in_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);

		if (n > 0) {
			new_job = (struct host_job *)
				malloc(sizeof(struct host_job));
			new_job->in_port_index = k;
			new_job->packet = in_packet;

			job_q_add(&job_q, new_job);

			flag = 1;
		}
		else {
			free(in_packet);
		}
	}
  
	flag = 0;

	if (job_q_num(&job_q) > 0) {

		new_job = job_q_remove(&job_q);

		int vport = -1;
		for (i=0; i<MAX_FWD_LENGTH; i++)
		{
			//scan for valid match
			if (fwd_table[i].valid && fwd_table[i].dst == new_job->packet->src)
			{
				vport = fwd_table[i].port;
				break;
			}
		}
		if (vport == -1) //need to add an entry
		{
			for (i=0; i<MAX_FWD_LENGTH; i++)
			{
				if (!fwd_table[i].valid)
				{
					fwd_table[i].valid = 1;
					fwd_table[i].dst = new_job->packet->src;
					fwd_table[i].port = new_job->in_port_index;
					vport = fwd_table[i].port;
					break;
				}
			}
		}

		vport = -1;
		for (i=0; i<MAX_FWD_LENGTH; i++)
		{
			//scan for valid match
			if (fwd_table[i].valid && fwd_table[i].dst == new_job->packet->dst)
			{
				vport = fwd_table[i].port;
				break;
			}
		}

		if (vport > -1)
		{
			packet_send(node_port[vport], new_job->packet);
		}
		else
		{
			for (k=0; k<node_port_num; k++)
			{
				if (k != new_job->in_port_index) {
					packet_send(node_port[k], new_job->packet);
				}
			}
		}

		free(new_job->packet);
		free(new_job);
	}
	usleep(TENMILLISEC);

}

}




