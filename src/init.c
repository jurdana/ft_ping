#include "../header/ping.h"

void	init(t_ping *ping)
{
	ping->packet_size = 56;
	ping->rtt_min = DBL_MAX;
	ping->linger = 10;
}


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

bool	is_number(char *str)
{
	int i = 0;

	if (!str || !str[0])
		return (false);
	while(str[i])
	{
		if (!isdigit(str[i]))
			return (false);
		i++;
	}
	return (true);
}


int parse_number(char *optarg, long min, long max)
{
	long	val;
	char	*endptr;

	val = strtol(optarg, &endptr, 10);
	if (*endptr != '\0' || endptr == optarg)
	{
		fprintf(stderr, "ft_ping: invalid value (`%s' near `%s')\n", optarg, endptr);
		exit(1);
	}
	if (val > max || val < min)
	{
		fprintf(stderr, "ft_ping: option value too big: %s\n", optarg);
		exit(1);
	}
	return (int)val;
}

void	parse_args(int argc, char **argv, t_ping *ping)
{
	int opt;
	opterr = 0;

	while ((opt = getopt(argc, argv, ":vnc:s:w:W:")) != -1)
	{
		switch (opt) {
			case 'v': ping->verbose = true; break;
			case 'n': ping->numeric = true; break;
			case 'c': ping->count = parse_number(optarg, 0, INT_MAX); ping->has_count = true; break;
			case 's': ping->packet_size = parse_number(optarg, 0, 65399); break;
			case 'w': ping->timeout = parse_number(optarg, 0, INT_MAX); ping->has_timeout = true; break;
			case 'W': ping->linger = parse_number(optarg, 0, INT_MAX); break;
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
