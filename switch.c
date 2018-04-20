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
#define MAX_SIZE_TABLE 100

#define LOOPS_BEFORE_TREE_PACKET 10

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

int localRootID = switch_id;
int localRootDist = 0;
int localParent = -1;
int *localPortTree = malloc(node_port_num* sizeof(int));
memset(localPortTree, 0, node_port_num);

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

int count = 0;
int doublecount = 0;
while(1) {

	// Send a tree packet only once every x times through the loop
	if (count <= 0)
	{
		// setup tree job
		new_job = (struct host_job *) malloc(sizeof(struct host_job));
		new_job->type = JOB_TREE_SEND;
		
		// setup tree packet
		new_packet = (struct packet *) malloc(sizeof(struct packet));
		new_packet->type = (char) PKT_TREE;
		new_packet->src = (char) switch_id;
		new_packet->length = 4;
		
		new_packet->payload[0]=(char) localRootID;
		new_packet->payload[1]=(char) localRootDist;
		new_packet->payload[2]= 'S';  // we're a switch
		new_job->packet = new_packet;
		
		// add the job to the queue
		job_q_add(&job_q, new_job);
		
		//reset the count
		count = LOOPS_BEFORE_TREE_PACKET;
		/* This was for a test
		doublecount++;
		if (doublecount == 50)
		{
			//printf("The tree table for Switch %d is as follows:\n", switch_id);
			int i;
			for( i = 0; i < net_getNumberOfNodes(); i++)
			{	
				if(localPortTree[i] == 1)
					printf("Node %d is part of Switch %d's tree\n", i, switch_id);
			}
			
			printf("Switch%d's parent is Node%d\n",switch_id, localParent);
		}
		*/
		
	} else {
		count--;
	}

	for (k = 0; k < node_port_num; k++) { /* Scan all ports */
		in_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);
		if ((n > 0) ) {
			new_job = (struct host_job *) 
				malloc(sizeof(struct host_job));
			new_job->in_port_index = k;
			if (in_packet->type == PKT_TREE)
				new_job->type = JOB_TREE_RECV;
			else
				new_job->type = JOB_SWITCH;
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
			
			switch (new_job->type)
			{
				case JOB_SWITCH:
					//====  packet->type = (char) PKT_TREE  or regular  =====
					if(new_job->packet->type == (char) PKT_TREE){
						printf("Switch sends job to all ports");
						for (k=0; k<node_port_num; k++) {
						//	if(new_job->in_port_index != k && localPortTree[k] == 1)
						//	if(new_job->in_port_index != k) 
							packet_send(node_port[k], new_job->packet);
						}
					}else{
						i = 1;
						while(forwarding_table[VALID][i]!=0){
							if((forwarding_table[DESTINATION][i] == new_job->packet->src) &&(new_job->in_port_index == forwarding_table[PORT][i])){
								src = forwarding_table[DESTINATION][i];
								printf("find record host : %d \n", src);
							}
							if(forwarding_table[DESTINATION][i] == new_job->packet->dst){
								target = forwarding_table[PORT][i];
								printf("find target host's port : %d \n", target);
							}
							i++;
						}
						if(target >= 0){
							printf("Send job to %d \n", target);
							packet_send(node_port[target], new_job->packet);
						}else{//try
							printf("Switch sends job to all ports");
							for (k=0; k<node_port_num; k++) {
								//if(new_job->in_port_index != k && localPortTree[k] == 1)
								if( new_job->in_port_index != k && localPortTree[k] == 1)	{
									packet_send(node_port[k], new_job->packet);
									printf("sent to port %d, and it is in the tree\n",k);
								} 
							}
						}
						if(src < 0 ){
							forwarding_table[VALID][i] = 1;
							forwarding_table[DESTINATION][i] = new_job->packet->src;
							printf("add src %d to table \n", new_job->packet->src);
							forwarding_table[PORT][i] = new_job->in_port_index;
							printf("add port %d to table \n", new_job->in_port_index);
							table_size ++;
						}
					
						
						
						//============
					}
//					printf("Switch %d's Table:\n", switch_id);
//					print_table(forwarding_table);
					break;
				
				case JOB_TREE_RECV:
					if (new_job->packet->payload[2] == 'S')  // The tree packet came from a switch
					{
						if ((int) new_job->packet->payload[0] < localRootID)  // Found a better root
						{
							localRootID = (int) new_job->packet->payload[0];
							if(localParent != -1 )  localPortTree[localParent]=0;  //0->1
							localParent = new_job->in_port_index;
							localRootDist = (int) new_job->packet->payload[1] + 1;
						} 
						else if ((int) new_job->packet->payload[0] == localRootID) // Found the same root
						{
							if (localRootDist > (int) new_job->packet->payload[1] + 1) // but it is closer this way
							{
								if(localParent != -1 )localPortTree[localParent]=0;  //0->1
								localParent = new_job->in_port_index;
								localRootDist = (int) new_job->packet->payload[1] + 1;
							}
						}
					}
					
					// Setup the localPortTree here:
					if (new_job->packet->payload[2] == 'H') // If we get a tree packet from a host it is part of the tree
					{
						localPortTree[new_job->in_port_index] = 1;
					} 
					else if (new_job->packet->payload[2] == 'S') // if we get a tree packet from a switch:
					{
						if (localParent == new_job->in_port_index) // if it's our parent it's part of the tree
							localPortTree[new_job->in_port_index] = 1;
						else if (new_job->packet->payload[3] == 'Y') // if it's our child it's part of the tree
						{
							localPortTree[new_job->in_port_index] = 1;
						}
						else
							localPortTree[new_job->in_port_index] = 0; // if it's neither it isn't part of our tree
					}
					else
						localPortTree[new_job->in_port_index] = 0; // broken base case, shouldn't get here ever
					
				break;
				
				case JOB_TREE_SEND:
					//TODO: Send on all links
					// set new_job->new_packet->.payload[3] to 'Y' if localParent == the destination id, or 'N' otherwise
					for (k=0; k<node_port_num; k++) 
					{
						//printf("Sending a Tree Packet to %d\n", node_port[k]->pipe_host_id);

						if (localParent == k)
							new_job->packet->payload[3] = 'Y';
						else
							new_job->packet->payload[3] = 'N';
 						/*
						if(switch_id = 6)
						{
							//printf("Switch%d's localRootID is %d\n", switch_id, localRootID);
							printf("Switch%d sent the following payload:\n", switch_id);
							printf("  Payload[0] (localRootID) = %d\n", (int) new_packet->payload[0]);
							printf("  Payload[1] (localRootDist) = %d\n", (int) new_packet->payload[1]);
							printf("  Payload[2] (node type) = %c\n", new_packet->payload[2]);
							printf("  Payload[3] (is Parent?) = %c\n", new_packet->payload[3]);
						}
						 */
						packet_send(node_port[k], new_job->packet);
					}
				break;
			}
				
	//		printf("Target: %d, Src: %d\n", target, src);
	//		printf("Table size: %d\n", table_size);
	//		print_table(forwarding_table);
			free(new_job->packet);
			free(new_job);
		}//end if (n)

	usleep(TENMILLISEC);

} /* End of while loop */

}




