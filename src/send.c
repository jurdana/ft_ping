#include "../header/ping.h"

/*
send_packet - builds a fresh ICMP echo request into buffer (new
sequence number, timestamp and checksum) and sends it to the resolved
destination with sendto. On success the transmitted counter is
incremented; on failure it prints a sendto error and leaves the counter
unchanged.
*/
void	send_packet(t_ping *ping, char *buffer)
{
	build_packet(ping, buffer);

	ssize_t sent = sendto(ping->sockfd, buffer, sizeof(struct icmphdr) + ping->packet_size,
		0, (struct sockaddr *)&ping->dest_addr, sizeof(ping->dest_addr));
	
	if (sent < 0)
		perror("ft_ping: sendto failure");
	else
		ping->transmitted++;
}
