#include "../header/ping.h"

void	receive_packet(t_ping *ping)
{
	char recv_buffer[1024];
	ssize_t recv_len = recvfrom(ping->sockfd, recv_buffer, sizeof(recv_buffer), 0, NULL, NULL);
	
	if (recv_len <= 0)
		return ;

	decode_packet(ping, recv_buffer, recv_len);
}

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


void	decode_packet(t_ping *ping, char *recv_buffer, ssize_t recv_len)
{
	struct iphdr	*ip = (struct iphdr*)recv_buffer;
	int				ip_header_len = ip->ihl * 4;

	if (recv_len < ip_header_len + (int)sizeof(struct icmphdr))
		return;

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
	}
	else if (type == ICMP_TIME_EXCEEDED || type == ICMP_DEST_UNREACH)
	{
		if (ping->verbose)
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
			printf("IP Hdr Dump:\n");
			unsigned char *orig = (unsigned char *)(recv_buffer + ip_header_len + sizeof(struct icmphdr));
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
	}
}
