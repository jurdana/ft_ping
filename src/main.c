#include "../header/ping.h"

volatile sig_atomic_t g_stop = 0;

/*
handle_signal - SIGINT (CTRL+C) handler. Sets the global stop flag so
the main loop exits cleanly and prints the final statistics. THe signal
number is unused.
*/
void	handle_signal(int signal)
{
	(void)signal;
	g_stop = 1;
}


/*
loop - main ping loop. Records the start time, then repeatdefly sends an
echo request, waits for a reply, and increments the sequence number,
pausing one second between iterations. Stops when SIGINT is caught, when
the -c packet count is reached, or when the -w global timeout exprires,
then prints the statistics.
*/
void	loop(t_ping *ping)
{
	char buffer[sizeof(struct icmphdr) + ping->packet_size];

	gettimeofday(&ping->start_time, NULL);

	while (!g_stop)
	{
		send_packet(ping, buffer);
		receive_packet(ping);
		ping->seq++;
		
		if(ping->has_count && ping->transmitted >= ping->count)
			break;
		
		struct timeval now;
		gettimeofday(&now, NULL);
		if (ping->has_timeout && (now.tv_sec - ping->start_time.tv_sec) >= ping->timeout)
			break;
		
		sleep(1);
	}

	print_stats(ping);
}


/*
main - programs entry point. Zero-initialises the t_ping state, applies
defaults (init), parses arguments, resolves the host, opens the raw
socket, installs the SIGINT handler, prints the PING header and runs the
loop. returns 0 if at least one reply was received, 1 otherwise.
*/
int main(int argc, char **argv)
{

	t_ping ping = {0};

	init(&ping);

	parse_args(argc, argv, &ping);

	init_hostname(&ping);

	init_socket(&ping);

	signal(SIGINT, handle_signal);

	print_header(&ping);

	loop(&ping);

	if (ping.received > 0)
		return (0);
	return (1);
}
