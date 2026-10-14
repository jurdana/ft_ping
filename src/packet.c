#include "../header/ping.h"

/*
checksum - computes the 16-bit Internet checksum (RFC 1071) over len
bytes: sums the data as 16-bit words, adds the trailing byte if the
length is odd, folds the carries back into the low 16 bits, and returns
the one's complement of the result.
*/
uint16_t	checksum(void *data, int len)
{
	uint16_t *buffer = data;
	uint32_t sum = 0;

	for (; len > 1; len -= 2)
		sum += *buffer++;

	if (len == 1)
		sum += *(uint8_t *)buffer;

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return (uint16_t)~sum;
}

/*
build_packet - fills the ICMP echo request in buffer: a payload of
packet_size bytes set to 0,1,2,... with a gettimeofday timestamp written
at its start when it is large enough (used to compute the RTT on reply),
then the ICMP header (type 8, id and sequence in network byte order).
The checksum field is zeroed and then computed over the whole packet.
*/
void	build_packet(t_ping *ping, char *buffer)
{

	struct icmphdr	*hdr = (struct icmphdr *)buffer;
	char			*payload = buffer + sizeof(struct icmphdr);
	int				total_size = sizeof(struct icmphdr) + ping->packet_size;

	for (int i = 0; i < ping->packet_size; i++)
		payload[i] = i;

	if (ping->packet_size >= (int)sizeof(struct timeval))
	{
		struct timeval *tv = (struct timeval *)payload;
		gettimeofday(tv, NULL);
	}

	hdr->type = ICMP_ECHO;
	hdr->code = 0;
	hdr->un.echo.id = htons(ping->id);
	hdr->un.echo.sequence = htons(ping->seq);
	hdr->checksum = 0;
	hdr->checksum = checksum(buffer, total_size);
}
