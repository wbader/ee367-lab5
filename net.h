

int net_init();

struct man_port_at_man *net_get_man_ports_at_man_list();
struct man_port_at_host *net_get_host_port(int host_id);

struct net_node *net_get_node_list();
struct net_port *net_get_port_list(int host_id);

// I created this to make a linked list of pipes 
struct pipe_link {
	int pipe[2];
	int conector;
	struct pipe_link * next;
};

