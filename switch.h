/*TO EDIT NOT FINAL!!!*/

#define NUMSWITCHS 100
#define NUMSWITCHLINKS 200

typedef struct pqnode {
   packetBuffer packet;
   int k;
   struct pqnode * next;
} pqnode;

typedef struct {
   pqnode * head;
   pqnode * tail;
} pqueue;

typedef struct {
   int toHost[2];
   int fromHost[2];
} switchLink;

typedef struct { /* State of host */
   int   physid;              /* physical id */
   int   rcvflag;
   int   numlinks;
   packetBuffer sendPacketBuff;  /* send packet buffer */
   packetBuffer rcvPacketBuff;
   switchLink sLink;
   LinkInfo linkin[NUMSWITCHLINKS];           /* Incoming communication link */
   LinkInfo linkout[NUMSWITCHLINKS];          /* Outgoing communication link */
} switchState;

typedef struct {
   int numlinks;
   switchLink link[NUMSWITCHLINKS];
} switchLinkArrayType;

typedef struct {
   int valid;
   int destNetworkAddress;
   LinkInfo outlink;
} tableEntry;

typedef struct {
   int numentries;
   tableEntry Entry[NUMSWITCHLINKS];
} forwardingTable;

void switchMain(switchState * sstate);

void switchInit(switchState * sstate, int physid);

pqueue * init(void);
pqnode * pop(pqueue * q);
void push(pqueue * q,packetBuffer p,int k);
