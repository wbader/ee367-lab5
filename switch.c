 /*
  *switch.c
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
#include "packet.h"
//#include "switch.h"

#define MAX_FILE_BUFFER 1000
#define MAX_MSG_LENGTH 100
#define MAX_DIR_NAME 100
#define MAX_FILE_NAME 100
#define PKT_PAYLOAD_MAX 100
#define TENMILLISEC 10000   /* 10 millisecond sleep */

#define VALID 0
#define DESTINATION 1
#define PORT 2
#define MAX_SIZE_TABLE 10

void initialize_table(int forwarding_table[][MAX_SIZE_TABLE]){
	int i,j;
	for(i=1; i<MAX_SIZE_TABLE; i++){
		for(j=0;j<3;j++){
			forwarding_table[j][i]=0;
		}
	}
}
void print_table(int forwarding_table[][MAX_SIZE_TABLE]){
	int i,j;
		for(i=1; i<MAX_SIZE_TABLE; i++){
		for(j=0;j<3;j++){
			printf("Table: %d\n", forwarding_table[j][i]);
		}
		printf("\n");
	}
}




void switch_main(int switch_id)
{

struct net_port *node_port_list; 
struct net_port **node_port;  // Array of pointers to node ports
int node_port_num;            // Number of node ports


int i,j, k, n;
int dst, src, port, target;
int table_size = 0;
int forwarding_table[3][MAX_SIZE_TABLE];
initialize_table(forwarding_table);

struct packet *in_packet; /* Incoming packet */
struct packet *new_packet;
struct net_port *p;
struct host_job *new_job;
struct host_job *new_job2;
struct job_queue job_q;
node_port_list = net_get_port_list(switch_id);
node_port_num = 0;
for (p=node_port_list; p!=NULL; p=p->next) {
	node_port_num++;
}
node_port = (struct net_port **) 
	malloc(node_port_num*sizeof(struct net_port *));
p = node_port_list;
for (k = 0; k < node_port_num; k++) {
	node_port[k] = p;
	p = p->next;
}	

/* Initialize the job queue */
job_q_init(&job_q);

while(1) {

	for (k = 0; k < node_port_num; k++) { /* Scan all ports */
		in_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);
		if ((n > 0) ) {
			new_job = (struct host_job *) 
				malloc(sizeof(struct host_job));
			new_job->in_port_index = k;
			new_job->packet = in_packet;
			job_q_add(&job_q, new_job);
		}
	}
	/*
 	 * Execute one job in the job queue
 	 */
	dst = src = port = target = -1;
	if (job_q_num(&job_q) > 0) {
			new_job = job_q_remove(&job_q);
//====  in   =====
			i = 1;
			while(forwarding_table[VALID][i]!=0){
				if((forwarding_table[DESTINATION][i] == new_job->packet->src) &&(new_job->in_port_index == forwarding_table[PORT][i])){
					src = forwarding_table[DESTINATION][i];
//					printf("find record host : %d \n", src);
				}
				if(forwarding_table[DESTINATION][i] == new_job->packet->dst){
					target = forwarding_table[PORT][i];
//					printf("find target host's port : %d \n", target);
				}
				i++;
			}
			if(target >= 0){
//				printf("Send job to %d \n", target);
					packet_send(node_port[target], new_job->packet);
			}else{//try
//				printf("Switch sends job to all ports");
				for (k=0; k<node_port_num; k++) {
					if(new_job->in_port_index != k)	packet_send(node_port[k], new_job->packet);
				}
			}
			if(src < 0){
				forwarding_table[VALID][i] = 1;
				forwarding_table[DESTINATION][i] = new_job->packet->src;
//				printf("add src %d to table \n", new_job->packet->src);
				forwarding_table[PORT][i] = new_job->in_port_index;
//				printf("add port %d to table \n", new_job->in_port_index);
				table_size ++;
			}
	//		printf("Target: %d, Src: %d\n", target, src);
	//		printf("Table size: %d\n", table_size);
	//		print_table(forwarding_table);
	//		free(new_job->packet);
	//		free(new_job);
		}//end if (n)

	usleep(TENMILLISEC);

} /* End of while loop */

}




