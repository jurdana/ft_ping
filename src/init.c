#include "../header/ping.h"


/*
init - sets the default configuration values before argument parsing.
packet_size (56) and linger (10s) are inetutils' defaults, overwritten
if -s / -W are given. rtt_min starts at DBL_MAX so the first measured
RTT always becomes the minimum.
*/
void	init(t_ping *ping)
{
	ping->packet_size = 56;
	ping->rtt_min = DBL_MAX;
	ping->linger = 10;
}

/*
print_help - prints the usage/help message and the list of supported
options, then returns. Called when -? is passed.
*/
void	print_help(void)
{
	printf("Usage: ft_ping [OPTION...] HOST ...\n");
	printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
	printf("\n");
	printf("  -c NUMBER                  stop after sending NUMBER packets\n");
	printf("  -n                         do not resolve host addresses\n");
	printf("  -s NUMBER                  send NUMBER data octets\n");
	printf("  -v                         verbose output\n");
	printf("  -w N                       stop after N seconds\n");
	printf("  -W N                       number of seconds to wait for response\n");
	printf("  -?                         give this help list\n");
}

/*
parse_number - converts a numeric option argument to an int using
strtol, validation it against [min, max]. Exits with code 1 and an
inetutils-style error on non-numeric input, residue after the number,
a value above max (or negative), or a value below min.
Returns the parsed value as an int on success.
*/
int parse_number(char *optarg, long min, long max)
{
    long    val;
    char    *endptr;

    val = strtol(optarg, &endptr, 10);
    if (*endptr != '\0' || endptr == optarg)
    {
        fprintf(stderr, "ft_ping: invalid value (`%s' near `%s')\n", optarg, endptr);
        exit(1);
    }
    if (val < 0 || val > max)
    {
        fprintf(stderr, "ft_ping: option value too big: %s\n", optarg);
        exit(1);
    }
    if (val < min)
    {
        fprintf(stderr, "ft_ping: option value too small: %s\n", optarg);
        exit(1);
    }
    return (int)val;
}

/*
parse_args - parses the command linme options with getopt and fills the
t_ping config (verbose, numeric, count, packet_size, timeout, linger).
Handles -? (help, exit 0), missing option arguments and unknown
options (usage error, exit 64). Stores the destination host or exits
64 if no host operand is given.
*/
void	parse_args(int argc, char **argv, t_ping *ping)
{
	int opt;
	opterr = 0;

	while ((opt = getopt(argc, argv, ":vnc:s:w:W:")) != -1)
	{
		switch (opt) {
			case 'v': ping->verbose = true; break;
			case 'n': ping->numeric = true; break;
			case 'c':
				ping->count = parse_number(optarg, 0, INT_MAX);
				if (ping->count > 0)
					ping->has_count = true;
				break;
			case 's': ping->packet_size = parse_number(optarg, 0, 65399); break;
			case 'w': ping->timeout = parse_number(optarg, 1, INT_MAX); ping->has_timeout = true; break;
			case 'W': ping->linger = parse_number(optarg, 1, INT_MAX); break;
			case ':':
				fprintf(stderr, "ft_ping: option requires an argument -- '%c'\n", optopt);
				fprintf(stderr, "Try 'ft_ping --help' or 'ft_ping --usage' for more information.\n");
				exit(64);
			case '?':
				if (optopt == '?' || optopt == 0)
				{
					print_help();
					exit(0);
				}
				fprintf(stderr, "ft_ping: invalid option -- '%c'\n", optopt);
				fprintf(stderr, "Try 'ft_ping --help' or 'ft_ping --usage' for more information.\n");
				exit(64);
		}
	}
	if (argv[optind])
	{
		ping->dest = argv[optind];
	}
	else
	{
		fprintf(stderr,"ft_ping: missing host operand\nTry 'ft_ping --help' or 'ft_ping --usage' for more information.\n");
		exit(64);
	}
}

/*
init_hostname - resolves the destination (IPv4 address or hostname) to
a sockaddr_in via getaddrinfo and stores it in ping->dest_addr. Exits
with code 1 and "unknow host" if resolution fails.
*/
void	init_hostname(t_ping *ping)
{
	int				status;
	struct addrinfo	hints, *res;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	status = getaddrinfo(ping->dest, NULL, &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, "ft_ping: unknown host\n");
		exit(1);
	}

	memcpy(&ping->dest_addr, res->ai_addr, sizeof(struct sockaddr_in));

	freeaddrinfo(res);
}


/*
init_socket - creates the raw ICMP socket (requires root) and dests its
options: SO_RCVTIMEO (receive timeout = linger/-W), SO_BROADCAST (to 
allow pinging broadcast addresses) and IP_TTL (outgoing TTL). Also sets
ping->id to the low 16 bits of the PID to recognise our own replies.
Exits with code 1 on any failure.
*/
void	init_socket(t_ping *ping)
{
	struct timeval tv;

	ping->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping->sockfd < 0)
	{
		fprintf(stderr, "ft_ping: Lacking privilege for icmp socket.\n");
		exit(1);
	}
	tv.tv_sec = ping->linger;
	tv.tv_usec = 0;
	if (setsockopt(ping->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		perror("ft_ping: setsockopt failure");
		exit (1);
	}
	int broadcast = 1;
	if (setsockopt(ping->sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
	{
		perror("ft_ping: setsockopt broadcast");
		exit(1);
	}
	int ttl = PING_TTL;
	if (setsockopt(ping->sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
	{
		perror("ft_ping: setsockopt ttl");
		exit(1);
	}
	ping->id = getpid() & 0xFFFF;
}
