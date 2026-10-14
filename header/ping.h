#ifndef PING_H
#define PING_H

#include <stdbool.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <limits.h>

/* Outgoing TTL for sent packets. Set to a low value (e.g. 1) to trigger
ICMP Time Exceeded replies and demonstrate -v verbose output; keep at 64
for normal operation.
*/
# define PING_TTL 64


/*
t_ping - holds the whole program state (one instance, on the stack in
main):
verbose/numeric/has_count/has_timeout : flags set from -v/-n/-c/-w
count/packet_size/timeout/linger      : option values (-c/-s/-w/-W)
id                                    : our ICMP id (pid & 0xFFFF)
sockfd                                : raw ICMP socket
seq                                   : current sequence number
transmitted/received                  : packet counters for stats
rtt_min/max/sum/sum_sq/rtt_count      : RTT accumulators for stddev
dest                                  : destination as given on the CLI
dest_addr                             : resolved destination address
start_time                            : loop start, used by -w timeout
*/
typedef struct s_ping
{
	bool	verbose;
	bool	numeric;
	bool	has_count;
	bool	has_timeout;

	int		count;
	int		packet_size;
	int		timeout;
	int		linger;
	int		id;
	int		sockfd;
	int		seq;
	int		transmitted;
	int		received;

	double	rtt_min;
	double	rtt_max;
	double	rtt_sum;
	double	rtt_sum_sq;
	int		rtt_count;
	
	char	*dest;

	struct sockaddr_in 	dest_addr;

	struct timeval		start_time;
}	t_ping;


// MAIN.C
void	handle_signal(int signal);
void	loop(t_ping *ping);

// INIT.C
void	init(t_ping *ping);
void	print_help(void);
int		parse_number(char *optarg, long min, long max);
void	parse_args(int argc, char **argv, t_ping *ping);
void	init_hostname(t_ping *ping);
void	init_socket(t_ping *ping);


// PACKET.C
uint16_t	checksum(void *data, int len);
void		build_packet(t_ping *ping, char *buffer);

// SEND.C
void	send_packet(t_ping *ping, char *buffer);

// RECEPTION.C
void	receive_packet(t_ping *ping);
void	format_host(t_ping *ping, struct iphdr *ip, char *out, size_t out_size);
int		decode_packet(t_ping *ping, char *recv_buffer, ssize_t recv_len);

// PRINT.C
void	print_header(t_ping *ping);
void	print_stats(t_ping *ping);

#endif