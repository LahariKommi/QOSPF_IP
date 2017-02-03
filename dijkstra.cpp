#include <iostream>
#include <vector>
#include <list>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <climits>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define SERVER_PORT 7899
#define DEFAULT_COST 10

using namespace std;

class Edge;

class Node
{
	public:
	uint32_t id;
	std::vector < Edge * > out_list;
	uint32_t distance;
	Node *parent;
};

class Edge
{
public:
	Node *src;
	Node *dest;
	uint32_t weight;
	uint32_t seqnum;
	std::string *src_ip_addr;
	std::string *dest_ip_addr;
};

struct sort_dist {
	bool operator()(const Node *a, const Node *b) {
		if (a->distance > b->distance) {
			return true;
		}
		else
			return false;
	}
} sort_distance;




void install_route(std::string destination, std::string nexthop, bool qos_table)
{
	if (destination == nexthop) {
		return;
	}
	std::string command("sudo ip route add ");
	command += destination;
	command += std::string(" via ");
	command += nexthop;
	if (qos_table) {
		command += std::string(" table QoSRT");
	}
	system(command.c_str());
	std::cout << command<< std::endl;
}

void flush_qos_table() {
	std::string command("sudo ip route flush table QoSRT");
	system(command.c_str());
}


class Graph
{
	public:

	std::vector <Node *> adj_list;
	std::vector <Edge *> edge_list;
	uint32_t numNodes;

	void insert_edge(uint32_t src, string src_addr, uint32_t dest, string dest_addr, uint32_t weight)
	{
		Node *u = adj_list[src];
		Node *v = adj_list[dest];
		Edge *new_edge = new Edge;
		new_edge->src = u;
		new_edge->dest = v;
		new_edge->weight = weight;
		new_edge->seqnum = 0;

		new_edge->src_ip_addr = new std::string(src_addr);
		new_edge->dest_ip_addr = new std::string(dest_addr);

		edge_list.push_back(new_edge);
		(u->out_list).push_back(new_edge);


		Edge *new_edge2 = new Edge;
		new_edge2->src = v;
		new_edge2->dest = u;
		new_edge2->weight = weight;
		new_edge2->seqnum = 0;
		new_edge2->src_ip_addr = new std::string(dest_addr);
		new_edge2->dest_ip_addr = new std::string(src_addr);
		edge_list.push_back(new_edge2);
		(v->out_list).push_back(new_edge2);
	}

	Graph(uint32_t n)
	{
		this->numNodes = n;
	    for (uint32_t i = 0; i < n; ++i) {
			Node *new_node = new Node;
			new_node->id = i;
			adj_list.push_back(new_node);
	    }
	}


	void find_shortest_path(uint32_t src)
	{
		std::vector <Node *> sorted_nodes;
		for(std::vector<Node *>::iterator i=adj_list.begin(); i!=adj_list.end(); i++)
		{
			sorted_nodes.push_back(*i);
			(*i)->distance= (uint32_t) -1;//std::numeric_limits<uint32_t>::max();
		}

	    Node * src_node = adj_list[src];
	    src_node->distance = 0;
	    src_node->parent = NULL;


	    while (!sorted_nodes.empty())
	    {
	        std::sort(sorted_nodes.begin(), sorted_nodes.end(), sort_distance);
	        Node *u = sorted_nodes.back();
	        sorted_nodes.pop_back();

	        for (std::vector< Edge * >::iterator  i = u->out_list.begin(); i != u->out_list.end(); ++i)
	        {
	            Node *v = (*i)->dest;
	            uint32_t weight = (*i)->weight;
	            if (v->distance > u->distance + weight)
	            {
	            	v->distance = u->distance + weight;
	            	v->parent = u;
	            }
	        }
	    }

	    printf("Vertex   Distance from Source\n");
	    for (uint32_t i = 0; i < adj_list.size(); ++i)
	        printf("%d \t\t %d\n", i, (adj_list[i])->distance);
	}

	std::string *find_next_hop_addr(uint32_t host_id) {
		Node *u, *v;

		if (host_id >= numNodes) {
			return new std::string("Error");
		}
		u = adj_list[host_id];
		v = u->parent;

		while(v->parent != NULL) {
			u = u->parent;
			v = u->parent;
		}
        for (std::vector< Edge * >::iterator  i = edge_list.begin(); i != edge_list.end(); ++i) {
        	if ((((*i)->dest)->id == u->id)  && (((*i)->src)->id == v->id)) {
        		return (*i)->dest_ip_addr;
        	}
        }
		return new std::string("Error");
	}

	int find_next_node_id(uint32_t src_id, std::string &ip_addr) {
		Node *u = adj_list[src_id];
		if (src_id >= numNodes) {
			return -1; // error
		}

        for (std::vector< Edge * >::iterator  i = u->out_list.begin(); i != u->out_list.end(); ++i) {
        	if (!(*i)->src_ip_addr->compare(ip_addr)) {
        		return ((*i)->dest)->id;
        	}
        }
        return -1; // error
	}

	int find_next_hops(uint32_t src_id, std::vector<std::string *> *ip_addrs) {
		Node *u = adj_list[src_id];
        for (std::vector< Edge * >::iterator  i = u->out_list.begin(); i != u->out_list.end(); ++i) {
        	ip_addrs->push_back((*i)->dest_ip_addr);
        }
        return -1;
	}


	void find_hosts(std::vector<Node *> &hosts) {
        for (std::vector< Node * >::iterator  i = adj_list.begin(); i != adj_list.end(); ++i) {
        	if ((*i)->out_list.size() == 1) {
            	hosts.push_back(*i);
        	}
        }
	}


	bool validate_seq(uint32_t self_id, uint32_t src_id, uint32_t dest_id, uint32_t seqnum) {
		Node *u = adj_list[src_id];
		if (self_id == src_id) {
			return false;
		}
        for (std::vector< Edge * >::iterator  i = u->out_list.begin(); i != u->out_list.end(); ++i) {
        	if (((*i)->dest)->id == dest_id) {
        		int last_recv_seq = (*i)->seqnum;
        		//Handle wrap around
        		if (seqnum > last_recv_seq) {
        			(*i)->seqnum = seqnum;
        			return true;
        		} else {
        			return false;
        		}

        	}
        }
        return false;
	}

	void modify_edge(uint32_t src_id, uint32_t dest_id, uint32_t cost) {
		Node *u = adj_list[src_id];
        for (std::vector< Edge * >::iterator  i = u->out_list.begin(); i != u->out_list.end(); ++i) {
        	if (((*i)->dest)->id == dest_id) {
        			(*i)->weight = cost;
        	}
        }
	}

	void install_route_table(uint32_t src, bool qos) {
		std::vector<Node *> hosts;
	    find_shortest_path(src);
		find_hosts(hosts);
		flush_qos_table();
        for (std::vector< Node * >::iterator  i = hosts.begin(); i != hosts.end(); ++i) {
        	uint32_t node_id = (*i)->id;
        	if (node_id == src) {
        		continue;
        	}
    		std::string *next_hop_addr = find_next_hop_addr(node_id);
        	std::string *dest_addr = (*i)->out_list[0]->src_ip_addr;
        	install_route(*dest_addr, *next_hop_addr, qos);
        }
	}

};

struct lsa_packet {
	uint32_t src_node_id;
	uint32_t dest_node_id;
	uint32_t seq_num;
	uint32_t cost;
};

Graph *construct_graph(std::ifstream &infile)
{
	std::string info, src_ip_addr, dst_ip_addr;
	uint32_t num_of_nodes, num_of_links, src_node_id, dest_node_id;
	infile >> info >> num_of_nodes;
	infile >> info >> num_of_links;
	infile.ignore();
	getline(infile, info);
	Graph *g = new Graph(num_of_nodes);

    for (uint32_t i = 0; i < num_of_links; i++) {
    	infile >> src_node_id >> src_ip_addr >> dest_node_id >> dst_ip_addr;
        g->insert_edge(src_node_id, src_ip_addr, dest_node_id, dst_ip_addr, DEFAULT_COST);
    	/*std::cout << " src_node_id " << src_node_id << " src_ip_addr " << src_ip_addr <<
    			" dest_node_id " << dest_node_id  << " dst_ip_addr " << dst_ip_addr << std::endl;*/
    }
    return g;
}

int create_server_socket(struct sockaddr_in *server_addr) {
	int server_socket = socket(PF_INET, SOCK_DGRAM, 0);

	if (server_socket < 0) {
		std::cerr << "Error opening socket";
		exit(1);
	}
	/*Configure settings in address struct*/
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(SERVER_PORT);
	server_addr->sin_addr.s_addr = INADDR_ANY;
	memset(server_addr->sin_zero, 0, sizeof(server_addr->sin_zero));
	int enable = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
	    std::cerr << "setsockopt failed";
	    exit(1);
	}

	/*Bind socket with address struct*/
	if (bind(server_socket, (struct sockaddr *) server_addr, sizeof(struct sockaddr_in)) < 0) {
	    std::cerr << "Bind failed" << strerror(errno);
	    exit(1);
	}
	return server_socket;
}

int server_socket;

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    std::cout << ("Closing socket\n");
    fflush(stdout);
    close(server_socket);
    exit(1);
}

int main(){
	int client_socket;
	struct lsa_packet lsa_received, lsa_to_be_sent;
	struct sockaddr_in server_addr, client_addr, receiver_addr;
	size_t addr_size = sizeof(client_addr);
	uint32_t seqnum = 0;

    signal(SIGINT, sigintHandler);

	std::ifstream topology_file("topology.txt");
	std::ifstream node_file("node_id.txt");
	if ((!topology_file.is_open()) || (!node_file.is_open())) {
		std::cerr << "File not found\n";
		return -1;
	}
	Graph *g = construct_graph(topology_file);
	uint32_t node_id;
	node_file >> node_id;
	g->install_route_table(node_id, false);
	g->install_route_table(node_id, true);

	std::vector<string *> next_hops;
	g->find_next_hops(node_id, &next_hops);
	vector<uint32_t> seq_received(g->numNodes);
	for (std::vector<string *>::iterator i= next_hops.begin(); i != next_hops.end(); i++) {
		std::cout << "Next hop IP address: " << *(*i) << std::endl;
	}

	/*Create UDP socket*/
	client_socket = socket(PF_INET, SOCK_DGRAM, 0);
	server_socket = create_server_socket(&server_addr);

	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons(SERVER_PORT);
	memset(receiver_addr.sin_zero, 0, sizeof(receiver_addr.sin_zero));

	while(1) {
		recvfrom(server_socket, (void *)&lsa_received, sizeof(struct lsa_packet), 0, (struct sockaddr *)&client_addr, (unsigned int *)&addr_size);

		char sender_addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), sender_addr, INET_ADDRSTRLEN);
		std::string sender_address(sender_addr);
		std::cout<< "New LSA Received "  << sender_addr << std::endl;

		if (sender_address == "127.0.0.1") {
			// timerlogic if not sending lsa
			lsa_to_be_sent.seq_num = htonl(++seqnum);
			lsa_to_be_sent.src_node_id = htonl(node_id);
			lsa_to_be_sent.cost = lsa_received.cost;
			char src_addr_str[INET_ADDRSTRLEN];
			struct in_addr src_addr;
			src_addr.s_addr = ntohl(lsa_received.src_node_id);
			inet_ntop(AF_INET, &(src_addr), src_addr_str, INET_ADDRSTRLEN);
			uint32_t cost = ntohl(lsa_received.cost);
			std::cout << "Src_addr : " << src_addr_str << std::endl;
			std::cout << "Cost : " << cost << std::endl;

			std::string src_addr_str2(src_addr_str);

			unsigned int dest_id = g->find_next_node_id(node_id, src_addr_str2);
			lsa_to_be_sent.dest_node_id = htonl(dest_id);
			g->modify_edge(node_id, dest_id, cost);
			g->install_route_table(node_id, true);

			for (std::vector<string *>::iterator i= next_hops.begin(); i != next_hops.end(); i++) {
				inet_pton(AF_INET, (*i)->c_str(), &(receiver_addr.sin_addr));
				sendto(client_socket, &lsa_to_be_sent, sizeof(struct lsa_packet), 0, (struct sockaddr *)&receiver_addr,sizeof(receiver_addr));
				std::cout << "Sending to:" << *(*i) << std::endl;
			}
			std::cout << "New LSA Sent with following Details" << std::endl;
			std::cout << "Source Node: " << node_id << std::endl;
			std::cout << "Dest Node: " << dest_id << std::endl;
			std::cout << "Sequence Number: " << seqnum << std::endl;
			std::cout << "Cost: " << cost << std::endl;
		} else {
			//Extract the fields from Received LSA messages.
			uint32_t src_id = ntohl(lsa_received.src_node_id);
			uint32_t dest_id = ntohl(lsa_received.dest_node_id);
			uint32_t cost = ntohl(lsa_received.cost);
			uint32_t seqnum = ntohl(lsa_received.seq_num);
			std::cout << "Received LSA Details" << std::endl;
			std::cout << "Source Node: " << src_id << std::endl;
			std::cout << "Dest Node: " << dest_id << std::endl;
			std::cout << "Sequence Number: " << seqnum << std::endl;
			std::cout << "Cost: " << cost << std::endl;
			if (!g->validate_seq(node_id, src_id, dest_id, seqnum)) {
				continue;
			}

			g->modify_edge(src_id, dest_id, cost);
			g->install_route_table(node_id, true);
			for (std::vector<string *>::iterator i= next_hops.begin(); i != next_hops.end(); i++) {
				if (sender_address == *(*i)) {
					continue;
				}
				inet_pton(AF_INET, (*i)->c_str(), &(receiver_addr.sin_addr));
				sendto(client_socket, &lsa_received, sizeof(lsa_received), 0, (struct sockaddr *)&receiver_addr,sizeof(receiver_addr));
				std::cout << "Forwarding to:" << *(*i) << std::endl;
			}
		}
	}

	return 0;
}

