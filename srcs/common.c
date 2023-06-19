#include "ft_ping.h"

uint16_t	ft_ntohs(uint16_t netshort) {
	if (ft_get_endianness() == FT_BIG_ENDIAN)
		return netshort;
	return (netshort << 8) | (netshort >> 8);
}

uint16_t	icmp_checksum(void *data, uint32_t len) {

	uint32_t	i = 0;
	uint32_t	dword_size = len / 2;
	uint32_t	sum = 0;
	uint16_t	*current_dword = (uint16_t*)data;
	uint16_t	odd_byte = 0;

	assert(len % 2 == 0); // We're just no gonna bother with the odd size case...
	while (i < dword_size) {
		sum += current_dword[i];

		i++;
	}
	if (len % 2 == 1) { // need one zero byte padding.
		*(uint8_t*)&odd_byte = *(uint8_t*)(data + len - 1); // The reason we do this dance is for byte order...
		sum += odd_byte;
	}

	// The reason we use an uint32_t is to count the number of carries over 16bits. We add this count back the original number
	// And if that sum produces a carry, we add it back again. That last sum shouldn't be able to produce a new carry.

	sum = (sum >> 16) + (sum & 0xffff);

	sum += (sum >> 16);
	return (uint16_t)~sum; // truncation to 16 bits
}


void	print_icmp_header(struct icmphdr *icmp, uint32_t size) {
	printf("ICMP: type %u, code %u, size %u", icmp->type, icmp->code, size);
	if (icmp->type == ICMP_ECHO || icmp->type == ICMP_ECHOREPLY) {
		printf(", id 0x%04x, seq 0x%04x", icmp->un.echo.id, icmp->un.echo.sequence);
	}
	printf("\n");
}


void	print_ip_header(struct ip *ip) {
	uint8_t	source[INET_ADDRSTRLEN];
	uint8_t	destination[INET_ADDRSTRLEN];
	
	inet_ntop(AF_INET, &ip->ip_src, (char *)source, sizeof(source));
	inet_ntop(AF_INET, &ip->ip_dst, (char *)destination, sizeof(destination));
	
	printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
	printf(" %1x  %1x  %02x %04x %04x   %1x %04x  %02x  %02x %04x %s  %s ",
	       ip->ip_v,
	       ip->ip_hl,
	       ip->ip_tos,
	       ft_ntohs(ip->ip_len), // So we need to ntohs this when we think it's appropriate...
	       ft_ntohs(ip->ip_id),
	       (ft_ntohs(ip->ip_off) & 0xe000) >> 13,
	       ft_ntohs(ip->ip_off) & IP_OFFMASK,
	       ip->ip_ttl,
	       ip->ip_p,
	       ft_ntohs(ip->ip_sum),
	       source,
	       destination);
	printf("\n");

	// Because when receiving a icmp error message, the data section contains the ip header + 8 bytes of it's data section
	// which in our case is the original icmp header we sent.
	struct icmphdr	*icmp_header = (struct icmphdr*)(ip + 1); //TODO use ip_header_len instead
	
	print_icmp_header(icmp_header, ft_ntohs(ip->ip_len) - sizeof(struct ip));
}

void	print_ip_header_with_dump(struct ip *ip) {
	printf("IP Hdr Dump:\n ");
	for (uint32_t	i = 0; i < sizeof(struct ip); i++) {
		uint8_t	current_byte = *((uint8_t *)ip + i);
		
		bool group = i % 2 == 0;

		char	*group_str;
		
		if (group)
			group_str = "";
		else
			group_str = " ";
		
		printf("%02x%s", current_byte, group_str);
	}
	printf("\n");
	print_ip_header(ip);
}
