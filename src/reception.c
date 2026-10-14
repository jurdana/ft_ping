#include "../header/ping.h"

/*
receive_packet - reads from the raw socket until a relevant one
is handled (our echo reply or an ICMP error), skipping packets that are
not for us (notably our own echo request echoed back on loopback). The
loop stops when recvfrom returns <= 0, which happens on the SO_RCVTIMEO
receive timeout.
*/
void	receive_packet(t_ping *ping)
{
	char	recv_buffer[1024];
	ssize_t	recv_len;

	while ((recv_len = recvfrom(ping->sockfd, recv_buffer,
			sizeof(recv_buffer), 0, NULL, NULL)) > 0)
	{
		if (decode_packet(ping, recv_buffer, recv_len) == 1)
			return ;
	}
}

/*
format_host - builds the textual representation of the sources address
into out. With -n (numeric) it writes the bare IP; otherwise it attempts
a reverse DNS lookup (getnameinfo) and writes "hostname (ip)", falling
back to the bare IP if resolution fails.
*/
void	format_host(t_ping *ping, struct iphdr *ip, char *out, size_t out_size)
{
	char				src_ip[INET_ADDRSTRLEN];
	char				hostname[NI_MAXHOST];
	struct sockaddr_in	src_addr;

	inet_ntop(AF_INET, &ip->saddr, src_ip, sizeof(src_ip));

	if (ping->numeric)
	{
		snprintf(out, out_size, "%s", src_ip);
		return ;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.sin_family = AF_INET;
	src_addr.sin_addr.s_addr = ip->saddr;

	if (getnameinfo((struct sockaddr *)&src_addr, sizeof(src_addr),
			hostname, sizeof(hostname), NULL, 0, 0) == 0)
			snprintf(out, out_size, "%s (%s)", hostname, src_ip);
	else
		snprintf(out, out_size, "%s", src_ip);
}

/*
decode_packet - interprets one received packet. it skips the IP header
(using IHL) to reach the ICMP header. For an echo reply carrying our id,
it updates the received count and RTT stats and prints the reply line
(with timne= only when the payload holds the timestamps), returning 1.
For an ICMP Time Exceeded or Destination Unreachable,it prints the error
line andm in verbose mode, a dump of the quoted original IP?ICMP header,
returning 1. Any other packet (too short, or not ours, e.g. our own echo
request) returns 0 so the caller keeps reading.
*/
int	decode_packet(t_ping *ping, char *recv_buffer, ssize_t recv_len)
{
	struct iphdr	*ip = (struct iphdr*)recv_buffer;
	int				ip_header_len = ip->ihl * 4;

	if (recv_len < ip_header_len + (int)sizeof(struct icmphdr))
		return (0);

	struct icmphdr	*icmp = (struct icmphdr *)(recv_buffer + ip_header_len);
	struct timeval	time_recv;

	gettimeofday(&time_recv, NULL);

	int		type = icmp->type;
	int		code = icmp->code;
	int		id = ntohs(icmp->un.echo.id);
	int		seq = ntohs(icmp->un.echo.sequence);
	int		ttl = ip->ttl;
	char	src_ip[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &ip->saddr, src_ip, sizeof(src_ip));

	if (type == ICMP_ECHOREPLY && id == ping->id)
	{
		ping->received++;

		if (recv_len - ip_header_len - (int)sizeof(struct icmphdr) >= (int)sizeof(struct timeval))
		{
			struct timeval *time_sent = (struct timeval *)(recv_buffer + ip_header_len + sizeof(struct icmphdr));
			double rtt = (time_recv.tv_sec - time_sent->tv_sec) * 1000.0
					+ (time_recv.tv_usec - time_sent->tv_usec) / 1000.0;

			ping->rtt_sum += rtt;
			ping->rtt_sum_sq += rtt * rtt;
			if (rtt < ping->rtt_min)
				ping->rtt_min = rtt;
			if (rtt > ping->rtt_max)
				ping->rtt_max = rtt;
			ping->rtt_count++;

			printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
					recv_len - ip_header_len, src_ip, seq, ttl, rtt);
		}
		else
		{
			printf("%ld bytes from %s: icmp_seq=%d ttl=%d\n",
					recv_len - ip_header_len, src_ip, seq, ttl);
		}
		return (1);
	}
	else if (type == ICMP_TIME_EXCEEDED || type == ICMP_DEST_UNREACH)
	{
		char	host[NI_MAXHOST + INET_ADDRSTRLEN + 4];
		char	*msg = NULL;

		format_host(ping, ip, host, sizeof(host));
		if (type == ICMP_TIME_EXCEEDED)
			msg = "Time to live exceeded";
		else if (type == ICMP_DEST_UNREACH)
		{
			if (code == ICMP_NET_UNREACH)
				msg = "Destination Net Unreachable";
			else if (code == ICMP_HOST_UNREACH)
				msg = "Destination Host Unreachable";
			else if (code == ICMP_PORT_UNREACH)
				msg = "Destination Port Unreachable";
			else
				msg = "Destination Unreachable";
		}
		printf("%ld bytes from %s: %s\n", recv_len - ip_header_len, host, msg);

		if (ping->verbose)
		{
			unsigned char *orig = (unsigned char *)(recv_buffer + ip_header_len + sizeof(struct icmphdr));
			printf("IP Hdr Dump:\n");
			printf(" ");
			for (int i = 0; i < 20; i++)
			{
				printf("%02x", orig[i]);
				if (i % 2 == 1)
					printf(" ");
			}
			printf("\n");
			printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
			struct iphdr *oip = (struct iphdr *)orig;
			printf(" %d  %d  %02x %04x %04x",
				oip->version, oip->ihl, oip->tos,
				ntohs(oip->tot_len), ntohs(oip->id));
			uint16_t frag = ntohs(oip->frag_off);
			printf("   %x %04x", frag >> 13, frag & 0x1FFF);
			char osrc[INET_ADDRSTRLEN];
			char odst[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &oip->saddr, osrc, sizeof(osrc));
			inet_ntop(AF_INET, &oip->daddr, odst, sizeof(odst));
			printf("  %02x  %02x %04x %s  %s \n",
				oip->ttl, oip->protocol, ntohs(oip->check), osrc, odst);
			struct icmphdr *oicmp = (struct icmphdr *)(orig + oip->ihl * 4);
			printf("ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n",
				oicmp->type, oicmp->code,
				ntohs(oip->tot_len) - (int)sizeof(struct iphdr),
				ntohs(oicmp->un.echo.id), ntohs(oicmp->un.echo.sequence));
		}
		return (1);
	}
	return (0);
}