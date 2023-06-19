#include "ft_ping.h"

#define PONG_USAGE "pong: <icmp_type> [ <icmp_code> ]\n"


struct {
	int	type;
	char	*name;
} icmp_types[] = {
	{ ICMP_ECHOREPLY, "echo-reply" },
	{ ICMP_DEST_UNREACH, "destination-unreachable" },       
	{ ICMP_SOURCE_QUENCH, "source-quench" },      
	{ ICMP_REDIRECT, "redirect" },           
	{ ICMP_ECHO, "echo-request" },               
	{ ICMP_TIME_EXCEEDED, "time-exceeded" },      
	{ ICMP_PARAMETERPROB, "parameter-prob" },      
	{ ICMP_TIMESTAMP, "timestamp" },          
	{ ICMP_TIMESTAMPREPLY, "timestamp-reply" },
	{ ICMP_INFO_REQUEST, "info-request" },       
	{ ICMP_INFO_REPLY, "info-reply" },         
	{ ICMP_ADDRESS, "address-request" },            
	{ ICMP_ADDRESSREPLY, "address-mask-reply" },       
};

uint8_t	get_icmp_type(char *type) {
	for (uint32_t i = 0; i < ITEM_COUNT(icmp_types); i++) {
		if (!strcmp(type, icmp_types[i].name))
			return icmp_types[i].type;
	}
	return ICMP_UNKNOWN_TYPE;
}

struct {
	int	type;
	int	code;
	char	*name;
} icmp_codes[] = {
	{ ICMP_DEST_UNREACH, ICMP_NET_UNREACH, "destination-net-unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, "destination-host-unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, "destination-protocol-unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, "destination-port-unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, "fragmentation-needed-and-df-set" },
	{ ICMP_DEST_UNREACH, ICMP_SR_FAILED, "source-route-failed" },
	{ ICMP_DEST_UNREACH, ICMP_NET_UNKNOWN, "network-unknown" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_UNKNOWN, "host-unknown" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_ISOLATED, "host-isolated" },
	{ ICMP_DEST_UNREACH, ICMP_NET_ANO, "net-ano" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_ANO, "host-ano" },			
	{ ICMP_DEST_UNREACH, ICMP_NET_UNR_TOS, "destination-network-unreachable-at-this-tos" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_UNR_TOS, "destination-host-unreachable-at-this-tos" },
	{ ICMP_DEST_UNREACH, ICMP_PKT_FILTERED, "pkt-filtered" },
	{ ICMP_DEST_UNREACH, ICMP_PREC_VIOLATION, "precedence-violation" },
	{ ICMP_DEST_UNREACH, ICMP_PREC_CUTOFF, "precedence-cutoff" },
	{ ICMP_REDIRECT, ICMP_REDIR_NET, "redirect-network" },
	{ ICMP_REDIRECT, ICMP_REDIR_HOST, "redirect-host" },
	{ ICMP_REDIRECT, ICMP_REDIR_NETTOS, "redirect-type-of-service-and-network" },
	{ ICMP_REDIRECT, ICMP_REDIR_HOSTTOS, "redirect-type-of-service-and-host" },
	{ ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, "time-to-live-exceeded" },
	{ ICMP_TIME_EXCEEDED, ICMP_EXC_FRAGTIME, "frag-reassembly-time-exceeded" },
	{ ICMP_PARAMPROB, ICMP_PARAMPROB_OPTABSENT, "requested-option-absent" },
};

uint8_t	get_icmp_code(char *code) {
	if (code == NULL) {
		return ICMP_DEFAULT_CODE;
	}
	for (uint32_t i = 0; i < ITEM_COUNT(icmp_codes); i++) {
		if (!strcmp(icmp_codes[i].name, code))
			return icmp_codes[i].code;
	}
	return ICMP_UNKNOWN_CODE;
}

void	print_available_options(void) {
	printf("Available modes: \n");
	for (uint32_t i = 0; i < ITEM_COUNT(icmp_types); i++) {
		uint32_t u;

		char	*type = icmp_types[i].name;
		uint32_t count = 0;
		for (u = 0; u < ITEM_COUNT(icmp_codes); u++) {
			if (icmp_codes[u].type == icmp_types[i].type) {
				char	*code = icmp_codes[u].name;
			
				printf("	pong %s %s\n", type, code);
				count++;
			}
		}

		if (count == 0)
			printf("	pong %s\n", type);
	}
}
	

int main(int argc, char **argv) {
	char	*type = argv[1];
	char	*code = argv[2];
	
	if (argc < 2) {
		dprintf(2, PONG_USAGE);
		print_available_options();
		return EXIT_FAILURE;
	}

	pid_t		pid = getpid();
	uint16_t	identity = pid & 0xFFFF;

	int	socket_fd;

	socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (-1 == socket_fd) {
		perror("socket(): ");
		return EXIT_FAILURE;
	}

	while (true) {
		struct msghdr		msg;
		struct sockaddr_in	source_address;
		int			receive_flags = 0;

		uint8_t			msg_buffer[4096]; // Way too big buffer for receiving the ECHO_REPLY...
		uint8_t			control_buffer[20480]; // sysctl net.core.optmem_max

		struct iovec		msg_iov = {
			.iov_base = msg_buffer,
			.iov_len  = sizeof(msg_buffer),
		};
	
		msg.msg_name = &source_address;
		msg.msg_namelen = sizeof(source_address);

		msg.msg_iov = &msg_iov;
		msg.msg_iovlen = 1;
	
		msg.msg_control = control_buffer;
		msg.msg_controllen = sizeof(control_buffer);
	
		msg.msg_flags = 0; // Could be set after call

		ssize_t	ret = recvmsg(socket_fd, &msg, receive_flags);


		if (-1 >= ret) {
			perror("recvmsg(): ");
			return EXIT_FAILURE;
		}
		
		size_t	packet_size_left = (size_t)ret;

		uint8_t source_address_hostname_str[NI_MAXHOST];
		uint8_t	source_address_raw_str[INET_ADDRSTRLEN];


		getnameinfo((const struct sockaddr *)&source_address, sizeof(source_address),
			    (char *)source_address_hostname_str, sizeof(source_address_hostname_str),
			    NULL, 0, 0);
		inet_ntop(AF_INET, &source_address.sin_addr,
			  (char *)source_address_raw_str, sizeof(source_address_raw_str));
		printf("----------------------------------------------------------\n");
		printf("Received icmp packet from %s (%s)\n", source_address_hostname_str, source_address_raw_str);


		if ((size_t)ret < ICMP_MINLEN + sizeof(struct timeval)) {
			dprintf(1, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
			dprintf(1, "Skipping this packet...");
			goto end_of_packet_processing;
		}

		struct icmphdr	*icmp_header;
		struct ip	*ip_header;

		uint8_t		*cursor	= msg_buffer;

		uint32_t	ip_header_len = 0;

		if (packet_size_left < sizeof(struct ip)) {
			dprintf(1, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
			dprintf(1, "Skipping this packet...");
			goto end_of_packet_processing;
		}
	
		ip_header = (struct ip*)(cursor);
		ip_header_len = ip_header->ip_hl << 2; // the size is given in qwords (uint32_t)

		cursor += ip_header_len;
		packet_size_left -= ip_header_len;

		if (packet_size_left < ICMP_MINLEN) {
			dprintf(1, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
			dprintf(1, "Skipping this packet...");
			goto end_of_packet_processing;
		}

		icmp_header = (struct icmphdr*)(cursor);
		printf("sequence %x == %x\n", icmp_header->un.echo.sequence, identity);

		if (icmp_header->un.echo.sequence == identity) { // Dropping our own packet
			dprintf(1, "Dropping our own packet\n");
			goto end_of_packet_processing;
		}
		print_icmp_header(icmp_header, (size_t)ret);

		uint8_t		icmp_message[sizeof (struct ip) + sizeof (struct icmphdr) * 2];
		uint32_t	packet_size;
		
		int		send_flags = 0;
		struct icmphdr	header;

		ft_bzero(icmp_message, sizeof(icmp_message));
		ft_bzero(&header, sizeof(header));
		
		header.type = get_icmp_type(type);
		header.code = get_icmp_code(code);
		header.checksum = 0;

		// Will use pid to distinguish between other pings' responses in SOCK_RAW mode
		// But we're gonna hide the identity within the sequence field
		// and reuse the original icmp id to dupe ping 
		
		header.un.echo.id = icmp_header->un.echo.id;
		header.un.echo.sequence = identity;
		
		uint32_t	offset = 0;
		
		ft_bzero(icmp_message, sizeof(icmp_message));
		
		ft_memcpy(icmp_message + offset, &header, sizeof(struct icmphdr));			
		offset += sizeof(struct icmphdr);
			
		ft_memcpy(icmp_message + offset, ip_header, ip_header_len);
		offset += ip_header_len;

		ft_memcpy(icmp_message + offset, icmp_header, sizeof(struct icmphdr));
		offset += sizeof(struct icmphdr);

		packet_size = offset;
		header.checksum = icmp_checksum(icmp_message, packet_size);

		ft_memcpy(icmp_message, &header, sizeof(struct icmphdr)); //sigh
		

		int bytes = sendto(socket_fd,
				   icmp_message,
				   packet_size,
				   send_flags,
				   (struct sockaddr*)&source_address,
				   sizeof(struct sockaddr_in));

		bool failure = bytes <= -1;
			
		if (failure) {
			perror("Sendto(): ");
			exit(EXIT_FAILURE);			
		}
		printf("Send back an ICMP response of type:\n");
		print_icmp_header(&header, packet_size);
	end_of_packet_processing:
		printf("----------------- END OF PACKET PROCESSING ---------------\n");
	}
}
