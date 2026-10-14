#include "../header/ping.h"

/*
print_header - prints the initial "PING host (ip): N data bytes" line.
In verbose mode (-v) it also appends the ICMP id in hex and decimal.
*/
void	print_header(t_ping *ping)
{
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ping->dest_addr.sin_addr, ip_str, sizeof(ip_str));

	printf("PING %s (%s): %d data bytes", ping->dest, ip_str, ping->packet_size);

	if (ping->verbose)
		printf(", id 0x%04x = %d", ping->id, ping->id);
	printf("\n");


}

/*
print_stats - prints the final statistics block: packets transmitted,
received and the packet-loss percentage. When at least one RTT was
measured, it also prints round-trip min/avg/max/stddev, where avg is the
mean, and stddev is derived from accumulated sum and sum of squares
of the RTTs (swrt of the variance).
*/
void	print_stats(t_ping *ping)
{
	double loss;
	double avg;
	double variance;
	double stddev;

	printf("--- %s ping statistics ---\n", ping->dest);

	if (ping->transmitted > 0)
		loss = (double)(ping->transmitted - ping->received) / ping->transmitted * 100.0;
	else
		loss = 0;

	printf("%d packets transmitted, %d packets received, %.0f%% packet loss\n",
		ping->transmitted, ping->received, loss);

	if (ping->rtt_count > 0)
	{
		avg = ping->rtt_sum / ping->rtt_count;
		variance = (ping->rtt_sum_sq / ping->rtt_count) - (avg * avg);
		stddev = sqrt(variance);
		printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			ping->rtt_min, avg, ping->rtt_max, stddev);
	}
}
