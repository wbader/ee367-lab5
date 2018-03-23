/*
 * Source code for the manager.  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "man.h"
#include "net.h"
#include "host.h"

#define MAXBUFFER 1000
#define PIPE_WRITE 1 
#define PIPE_READ  0
#define TENMILLISEC 10000
#define MAX_DNS_NAME_LENGTH 50
#define DELAY_FOR_HOST_REPLY 10  /* Delay in ten of milliseconds */

void display_host(struct man_port_at_man *list, 
			struct man_port_at_man *curr_host);
void change_host(struct man_port_at_man *list,
			struct man_port_at_man **curr_host);
void display_host(struct man_port_at_man *list, 
			struct man_port_at_man *curr_host);
void display_host_state(struct man_port_at_man *curr_host);
void set_host_dir(struct man_port_at_man *curr_host);
char man_get_user_cmd(int curr_host); 


/* Get the user command */
char man_get_user_cmd(int curr_host)
{
char cmd;

while(1) {
	/* Display command options */
   	printf("\nCommands (Current host ID = %d):\n",curr_host );
 	printf("   (s) Display host's state\n");
	printf("   (m) Set host's main directory\n");
	printf("   (h) Display all hosts\n");
	printf("   (c) Change host\n");
	printf("   (p) Ping a host\n");
	printf("   (u) Upload a file to a host\n");
	printf("   (d) Download a file from a host\n");
	printf("   (r) Register domain name\n");
	printf("   (q) Quit\n");
	printf("   Enter Command: ");
	do {
		cmd = getchar();
	} while(cmd == ' ' || cmd == '\n'); /* get rid of junk from stdin */

/* Ensure that the command is valid */
	switch(cmd)
	{
		case 's':
		case 'm':
		case 'h':
		case 'r':
		case 'c':
		case 'p':
		case 'u':
		case 'd':
		case 'q': return cmd;
		default: 
			printf("Invalid: you entered %c\n\n", cmd);
	}
}
}

/* Change the current host */
void change_host(struct man_port_at_man *list,
			struct man_port_at_man **curr_host)
{
int new_host_id;

// display_host(list, *curr_host);
printf("Enter new host: ");
scanf("%d", &new_host_id);
printf("\n");

/* Find the port of the new host, and then set it as the curr_host */
struct man_port_at_man *p;
for (p=list; p!=NULL; p=p->next) {
	if (p->host_id == new_host_id) {
		*curr_host = p;
		break;
	}
}
}

/* Display the hosts on the consosle */
void display_host(struct man_port_at_man *list, 
			struct man_port_at_man *curr_host)
{
struct man_port_at_man *p;

printf("\nHost list:\n");
for (p=list; p!=NULL; p=p->next) {
	printf("   Host id = %d ", p->host_id);
	if (p->host_id == curr_host->host_id) {
		printf("(<- connected)");
	}
	printf("\n");
}
}

/* 
 * Send command to the host for it's state.  The command
 * is a single character 's'
 *
 * Wait for reply from host, which should be the host's state.
 * Then display on the console. 
 */
void display_host_state(struct man_port_at_man *curr_host)
{
char msg[MAN_MSG_LENGTH];
char reply[MAN_MSG_LENGTH];
char dir[NAME_LENGTH];
int host_id;
int n;

msg[0] = 's';
msg[1] = '\0';

//printf("In display_host_state\n");

write(curr_host->send_fd, msg, 1);

//printf("Sent msg: %s\n", msg);

n = 0;
while (n <= 0) {
	usleep(TENMILLISEC);
	n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
}
reply[n] = '\0';
sscanf(reply, "%s %d", dir, &host_id);
printf("Host %d state: \n", host_id);
printf("    Directory = %s\n", dir);
}


void set_host_dir(struct man_port_at_man *curr_host)
{
char name[NAME_LENGTH];
char msg[NAME_LENGTH];
int n;

printf("Enter directory name: ");
scanf("%s", name);
n = sprintf(msg, "m %s", name);
write(curr_host->send_fd, msg, n);
}

/* 
 * Command host to send a ping to the host with id "curr_host"
 *
 * User is queried for the id of the host to ping.
 *
 * A command message is sent to the current host.
 *    The message starrts with 'p' followed by the id 
 *    of the host to ping.
 * 
 * Wiat for a reply
 */

void ping(struct man_port_at_man *curr_host)
{
char msg[MAN_MSG_LENGTH];
char reply[MAN_MSG_LENGTH];
char domain_name[MAX_DNS_NAME_LENGTH];
int host_to_ping;

int host_or_name;
int n,k;
printf("Ping by (1) Host ID or (2) Domain Name? \n");
scanf("%d", &host_or_name);

if(host_or_name == 2){
	printf("Enter name of host to ping: ");
	scanf("%s", domain_name);
	k = strlen(domain_name);
	domain_name[k] = '\0';
	printf("name you entered: %s\n", domain_name);
	n = sprintf(msg, "n %s", domain_name);
	write(curr_host->send_fd, msg, n);
	n = 0;
	while (n <= 0) {
		usleep(TENMILLISEC);
		n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
	}
	reply[n] = '\0';
	int ping_id = atoi(reply);
//	printf("This is from Man\n received id: %d\n start ping job\n",ping_id);
	n = sprintf(msg, "p %d", ping_id);
	if(ping_id == curr_host->host_id){
		printf("cannot ping itself\n");
	}else if(ping_id < 0){
		printf("This name has not registered\n");
	}else{
		write(curr_host->send_fd, msg, n);
		n = 0;
		while (n <= 0) {
			usleep(TENMILLISEC);
			n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
		}
		reply[n] = '\0';
		printf("%s\n",reply);
	}
	
}else if(host_or_name == 1){
	printf("Enter id of host to ping: ");
	scanf("%d", &host_to_ping);
	n = sprintf(msg, "p %d", host_to_ping);
	write(curr_host->send_fd, msg, n);
	n = 0;
	while (n <= 0) {
		usleep(TENMILLISEC);
		n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
	}
	reply[n] = '\0';
	printf("%s\n",reply);
	}else{printf("Invalid command\n");}

}

void register_domain_name(struct man_port_at_man *curr_host){
	char msg[MAN_MSG_LENGTH];
	char domain_name[MAX_DNS_NAME_LENGTH];
	int n;
	printf("Enter your domain name: (least than 50 characters)\n");
	scanf("%s", domain_name);
	n = sprintf(msg, "r %s", domain_name);
//	printf("length of string %d\n",n);
//	printf("%s\n", msg);
	write(curr_host->send_fd, msg, n);
}

/*
 * Command host to send a file to another host.
 *
 * User is queried for the
 *    - name of the file to transfer; 
 *        the file is in the current directory 'dir' 
 *    - id of the host to ping.
 *
 * A command message is sent to the current host.
 *    The message starrts with 'u' followed by the 
 *    -  id of the destination host 
 *    -  name of file to transfer
 */
int file_upload(struct man_port_at_man *curr_host)
{
int n;
int host_id;
char name[NAME_LENGTH];
char msg[NAME_LENGTH];

printf("Enter file name to upload: ");
scanf("%s", name);
printf("Enter host id of destination:  ");
scanf("%d", &host_id);
printf("\n");

n = sprintf(msg, "u %d %s", host_id, name);
write(curr_host->send_fd, msg, n);
usleep(TENMILLISEC);
}

int file_download(struct man_port_at_man *curr_host){
	int n,k;
	int host_id;
	int host_or_name;
	char domain_name[MAX_DNS_NAME_LENGTH];
	char reply[MAN_MSG_LENGTH];
	char name[NAME_LENGTH];
	char msg[NAME_LENGTH];
	printf("Download by (1) Host ID or (2) Domain Name? \n");
	scanf("%d", &host_or_name);
	if(host_or_name == 1){
	printf("Enter file name to download: ");
	scanf("%s", name);
	printf("Enter host id of destination:  ");
	scanf("%d", &host_id);
	printf("\n");
	n = sprintf(msg, "d %d %s", host_id, name);//send msg to host
	write(curr_host->send_fd, msg, n);
	usleep(TENMILLISEC);
	
	}else if(host_or_name == 2){
//========
	printf("Enter file name to download: ");
	scanf("%s", name);
	printf("Enter name of host to download: ");
	scanf("%s", domain_name);
	k = strlen(domain_name);
	domain_name[k] = '\0';
	printf("name you entered: %s\n", domain_name);
	n = sprintf(msg, "n %s", domain_name);
	write(curr_host->send_fd, msg, n);
	n = 0;
	while (n <= 0) {
		usleep(TENMILLISEC);
		n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
	}
	reply[n] = '\0';
	int download_id = atoi(reply);
//	printf("This is from Man\n received id: %d\n start ping job\n",download_id);
	n = sprintf(msg, "d %d %s", download_id, name);
	write(curr_host->send_fd, msg, n);
	
//========		
	}else{printf("Invalid command\n");}
	
	
}


/***************************** 
 * Main loop of the manager  *
 *****************************/
void man_main()
{

// State
struct man_port_at_man *host_list;
struct man_port_at_man *curr_host = NULL;

host_list = net_get_man_ports_at_man_list();
curr_host = host_list;

char cmd;          /* Command entered by user */

while(1) {
   /* Get a command from the user */
	cmd = man_get_user_cmd(curr_host->host_id);

   /* Execute the command */
	switch(cmd)
	{
		case 's': /* Display the current host's state */
			display_host_state(curr_host);
			break;
		case 'm': /* Set host directory */
			set_host_dir(curr_host);
			break;
		case 'h': /* Display all hosts connected to manager */
			display_host(host_list, curr_host);
			break;
		case 'c': /* Change the current host */
			change_host(host_list, &curr_host);
			break;
		case 'p': /* Ping a host from the current host */
			ping(curr_host);
			break;
		case 'u': /* Upload a file from the current host 
			     to another host */
			file_upload(curr_host);
			break;
		case 'd': /* Download a file from a host */
			file_download(curr_host);
			break;
		case 'r':
			register_domain_name(curr_host);
			break;
		case 'q':  /* Quit */
			return;
		default: 
			printf("\nInvalid, you entered %c\n\n", cmd);
	}
}   
} 


