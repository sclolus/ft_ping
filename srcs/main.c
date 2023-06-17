#include "ft_ping.h"


int		g_socket_fd;
uint32_t	ttl = 64;
uint32_t	sequence = 0;
uint32_t	duplicate_count = 0;
uint32_t	error_count = 0;
bool		current_sequence_already_responded = false;
bool		suppress_dup_packet_reporting = false;
bool		verbose = false;
bool		raw_socket = true;
char		*program_name = NULL;



static char	const **recvmsg_flags_strings(int flags) {
	static const struct {
		int	bits;
		char	*string;
	} flags_info[] = {
		{ MSG_EOR, "end-of-record" },
		{ MSG_TRUNC, "message truncated" },
		{ MSG_CTRUNC, "control messages truncated" },
		{ MSG_OOB, "out-of-band data received" },
		{ MSG_ERRQUEUE, "extended error in queue" },
	};
	
# define count (sizeof(flags_info) / sizeof(*flags_info)) // Merely because const compile-time-computable variables cannot be used as sizes for constant size arrays...
	
	static char const *returned_string[count + 1];

	uint32_t	i;
	for (i = count; i < count ; i++) {
		if (flags & flags_info[i].bits) {
			returned_string[i] = flags_info[i].string;
		}
	}

# undef count
	
	returned_string[i] = NULL;

	return returned_string;
}

static void ft_perror(char *header) {
	char *err = strerror(errno);
	
	dprintf(2, "%s: %s %s\n", program_name, header, err);
}

static double	timeval_to_double_ms(struct timeval time) {
	return (double)time.tv_sec * 1000 // A second is 1000 ms....
		+ (double)time.tv_usec / 1000; // a micro-second is 1/1000th of a ms....
}


int	set_socket_options(int fd) {
	int yes = 1;
	
	if (-1 == setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))) {
		ft_perror("setsockopt(IP_TTL)");
		return -1;
	}

	if (-1 == setsockopt(fd, IPPROTO_IP, IP_RECVERR, &yes, sizeof(yes))) {
		ft_perror("setsockopt(IP_RECVERR)");
		return -1;
	}

	if (-1 == setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &yes, sizeof(yes))) {
		ft_perror("setsockopt(IP_RECVTTL)");
		return -1;
	}
	
	if (-1 == setsockopt(fd, IPPROTO_IP, IP_RECVOPTS, &yes, sizeof(yes))) {
		ft_perror("setsockopt(IP_RECVTTL)");
		return -1;
	}
	
	if (-1 == setsockopt(fd, IPPROTO_IP, IP_RETOPTS, &yes, sizeof(yes))) {
		ft_perror("setsockopt(IP_RECVTTL)");
		return -1;
	}

	return 0;
}

const char	*pinged_address;
struct timeval	start;
double		min = INFINITY;
double		max = -INFINITY;
double		sum = 0.0;
double		sum_of_squares = 0.0;
uint64_t	packets_sent = 0;
uint64_t	packets_received = 0;
uint16_t	identity = 1;

enum {
	RAW_ADDRESS,
	HOSTNAME,
} argument_address_type;

int main(int argc, char **argv) {
	(void)recvmsg_flags_strings; // for debugging
	(void)argc;
	(void)argv;

	program_name = argv[0];
	
	int	opt_return;

	while (-1 != (opt_return = ft_getopt(argc, argv, "vh"))) {
		char	current_option = (char)opt_return;

		switch (current_option) {
		case 'v':
			#ifdef MODERN_PING
			suppress_dup_packet_reporting = false;
			#endif
			verbose = true;
			break;
		case 'h':
			dprintf(2, USAGE);
			exit(EXIT_FAILURE);
			break;
		case '?':
		default:
			dprintf(2, "\n%s", USAGE);
			exit(EXIT_FAILURE);
			break;
		}
	}
	

	if (argc <= g_optind) {
		dprintf(2, "ft_ping: usage error: Destination address required\n");
		exit(EXIT_FAILURE);
	}

	char *argument_address = argv[g_optind];

	pinged_address = argument_address;

	int socket_fd;
	int socket_type = SOCK_RAW;

	if (-1 == (socket_fd = socket(AF_INET, SOCK_RAW | SOCK_NONBLOCK, IPPROTO_ICMP))) { // First trying to get a raw socket.
		if (errno == EACCES || errno == EPERM) { // Trying to create DGRAM socket
			socket_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_ICMP);
			if (-1 == socket_fd) {
				if (errno == EACCES || errno == EPERM) {
					ft_perror("Lacking privilege for icmp socket.");
					exit(EXIT_FAILURE);
				}
				ft_perror("socket()");
				exit(EXIT_FAILURE);
			}
			socket_type = SOCK_DGRAM;
			raw_socket = false;
		} else {
			ft_perror("socket()");
			exit(EXIT_FAILURE);
		}
	}
	uint8_t	resolved_address[INET_ADDRSTRLEN];

	struct in_addr	   internet_address;
	struct sockaddr_in socket_address;

	int res;
	struct addrinfo *matched_addresses = NULL;

	res = inet_pton(AF_INET, argument_address, &internet_address);
	
	if (res == 0) {
		struct addrinfo hints = {
			.ai_family = AF_INET,
			.ai_socktype = socket_type,
			.ai_protocol = IPPROTO_ICMP,
			.ai_flags = 0,
		};
		
		int failure = getaddrinfo(argument_address, NULL, &hints, &matched_addresses);

		if (failure) {
			const char *error_string = gai_strerror(failure);
			
			dprintf(2, "ft_ping: %s: %s\n", argument_address, error_string);
			exit(EXIT_FAILURE);
		}

		argument_address_type = HOSTNAME;

		socket_address = *((struct sockaddr_in*)matched_addresses->ai_addr); //Should be struct sockaddr_in
		freeaddrinfo(matched_addresses);
	} else {
		socket_address.sin_addr = internet_address;
		socket_address.sin_family = AF_INET;
		socket_address.sin_port = 0; // Should not matter for icmp

		argument_address_type = RAW_ADDRESS;
	}

	inet_ntop(AF_INET, &socket_address.sin_addr, (char*)resolved_address, sizeof(resolved_address));

	
	if (-1 == set_socket_options(socket_fd)) {
		ft_perror("set_socket_options()");
		exit(EXIT_FAILURE);
	}

	g_socket_fd = socket_fd;
	signal(SIGINT, end);
	
	uint8_t		icmp_message[sizeof (struct ip) + sizeof (struct icmphdr) + DEFAULT_PAYLOAD_SIZE];
	const uint32_t	payload_size = DEFAULT_PAYLOAD_SIZE;
	const uint32_t	packet_size = sizeof (icmp_message);

	pid_t		pid = getpid();
	identity = pid & 0xFFFF;


	printf("PING %s (%s): %u data bytes", argument_address, resolved_address, payload_size);
	if (verbose) 
		printf(", id 0x%04x = %u", identity, identity);
	printf("\n");

	
	struct timeval prev;	
	gettimeofday(&start, NULL);

	prev = start;
	prev.tv_sec = 0; // We're just forcing the first packet to be send immediately;
	for (;;) {
		struct timeval	now;
		gettimeofday(&now, NULL);

		double		ms_diff = timeval_to_double_ms(now) - timeval_to_double_ms(prev);


		if (ms_diff >= 1000) { // we're due to send a packet.		
			int		flags = MSG_DONTWAIT;
			struct icmphdr	header;

			header.type = ICMP_ECHO;
			header.code = 0; // Code must be zero
			header.checksum = 0; // Set to zero before calculation

			if (raw_socket)
				header.un.echo.id = identity; // Will use pid to distinguish between other pings' responses in SOCK_RAW mode
			else
				header.un.echo.id = 0; // Will be filled in by the kernel in SOCK_DGRAM mode
			header.un.echo.sequence = sequence;

			char		*spid = ft_static_ulltoa(pid);
			uint32_t	offset = 0;

			ft_bzero(icmp_message, sizeof(icmp_message));

			ft_memcpy(icmp_message + offset, &header, sizeof(struct icmphdr));			
			offset += sizeof(struct icmphdr);
			
			ft_memcpy(icmp_message + offset, &now, sizeof(struct timeval));
			offset += sizeof(struct timeval);
			
			ft_memcpy(icmp_message + offset, STATIC_PAYLOAD, sizeof (STATIC_PAYLOAD));
			offset += sizeof(STATIC_PAYLOAD);
			
			ft_strcpy((char *)(icmp_message + offset - 1), spid);

			header.checksum = icmp_checksum(icmp_message, packet_size);

			ft_memcpy(icmp_message, &header, sizeof(struct icmphdr)); //sigh

			int bytes = sendto(socket_fd,
					   icmp_message,
					   packet_size,
					   flags,
					   (struct sockaddr*)&socket_address,
					   sizeof(struct sockaddr_in));

			bool failure = bytes <= -1;
			
			current_sequence_already_responded = false;
			if (failure) {
				if (errno == EACCES) {
					dprintf(2, "%sDo you want to ping broadcast? Then -b. If not, check your local firewall rules\n", ERROR_NAME_HEADER);
					exit(EXIT_FAILURE);
				}
				ft_perror("Sendto(): ");
				exit(EXIT_FAILURE);			
			}
			packets_sent++;
			sequence++;
			gettimeofday(&prev, NULL);
		}
		if (raw_socket) {
			receive_response();
		} else {
			receive_echo_reply();
			receive_error_message();
		}
		usleep(20);
	}
}

void	receive_echo_reply() {
	struct msghdr		msg;
	struct sockaddr_in	source_address;
	int			flags = MSG_DONTWAIT;

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

	ssize_t	ret = recvmsg(g_socket_fd, &msg, flags);


	if (-1 >= ret) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) { // Socket hasn't received packet.
			return ;
		}
		/* dprintf(2, "Silently ignoring error...\n"); */
		return ;
	}

	uint8_t source_address_hostname_str[NI_MAXHOST];
	uint8_t	source_address_raw_str[INET_ADDRSTRLEN];


	getnameinfo((const struct sockaddr *)&source_address, sizeof(source_address),
		    (char *)source_address_hostname_str, sizeof(source_address_hostname_str),
		    NULL, 0, 0);
	inet_ntop(AF_INET, &source_address.sin_addr,
		  (char *)source_address_raw_str, sizeof(source_address_raw_str));

	if ((size_t)ret < ICMP_MINLEN + sizeof(struct timeval)) {
		dprintf(2, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
		return ;
	}


	/* dprintf(2, "flags: %u\n", msg.msg_flags); */
	/* dprintf(2, "--- flags ---\n"); */
	/* for (char const **flag = recvmsg_flags_strings(msg.msg_flags); *flag != NULL; flag++) { */
	/* 	dprintf(2, "Flag returned by recvmsg: %s\n", *flag); */
	/* } */
	/* dprintf(2, "-------------\n"); */
	
	struct icmphdr	header;

	header = *((struct icmphdr*)msg_buffer);

	int	packet_ttl = ttl; // if not found resort to default ttl
	
	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		/* dprintf(2, "cmsg.level = %d, cmsg.type = %d\n", cmsg->cmsg_level, cmsg->cmsg_type); */
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
			ft_memcpy(&packet_ttl, CMSG_DATA(cmsg), sizeof(packet_ttl));

			/* dprintf(2, "Found ttl: %d!\n", packet_ttl);			 */
		}
	}


	if (header.type == ICMP_ECHOREPLY) {
		struct timeval	time_then = *(struct timeval *)(msg_buffer + sizeof(struct icmphdr));
		struct timeval	now;

		gettimeofday(&now, NULL);

		double	time_then_ms = timeval_to_double_ms(time_then);
		double	now_ms	     = timeval_to_double_ms(now);
		double	diff_ms	     = now_ms - time_then_ms;

		if (diff_ms <= min)
			min = diff_ms;
		if (diff_ms >= max)
			max = diff_ms;
		sum += diff_ms;
		sum_of_squares += diff_ms * diff_ms;
	

		char	*payload = (char*)msg_buffer + sizeof(struct icmphdr) + sizeof(struct timeval);

		(void)payload; // for debugging

		char	*extra = "";

		if (current_sequence_already_responded) {
			duplicate_count++;

			if (!suppress_dup_packet_reporting)
				extra = " (DUP!)";
		} else {
			packets_received++;
			current_sequence_already_responded = true;
		}

		if (argument_address_type == RAW_ADDRESS) {
			printf("%ld bytes from %s: icmp_seq=%u ttl=%u time=%3.3lf ms%s\n", ret, source_address_raw_str, header.un.echo.sequence, packet_ttl, diff_ms, extra);
		} else {
			printf("%ld bytes from %s (%s): icmp_seq=%u ttl=%u time=%3.3lf ms%s\n", ret, source_address_hostname_str, source_address_raw_str, header.un.echo.sequence, packet_ttl, diff_ms, extra);
		}
	} else {
		dprintf(2, "Response of unknown type received\n");
	}
}

struct {
	int	type;
	int	code;
	char	*description;
} icmp_specific_descriptions[] = {
	{ ICMP_DEST_UNREACH, ICMP_NET_UNREACH, "Destination Net Unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, "Destination Host Unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, "Destination Protocol Unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, "Destination Port Unreachable" },
	{ ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, "Fragmentation needed and DF set" },
	{ ICMP_DEST_UNREACH, ICMP_SR_FAILED, "Source Route Failed" },
	{ ICMP_DEST_UNREACH, ICMP_NET_UNKNOWN, "Network Unknown" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_UNKNOWN, "Host Unknown" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_ISOLATED, "Host Isolated" },
	{ ICMP_DEST_UNREACH, ICMP_NET_UNR_TOS, "Destination Network Unreachable At This TOS" },
	{ ICMP_DEST_UNREACH, ICMP_HOST_UNR_TOS, "Destination Host Unreachable At This TOS" },
	{ ICMP_REDIRECT, ICMP_REDIR_NET, "Redirect Network" },
	{ ICMP_REDIRECT, ICMP_REDIR_HOST, "Redirect Host" },
	{ ICMP_REDIRECT, ICMP_REDIR_NETTOS, "Redirect Type of Service and Network" },
	{ ICMP_REDIRECT, ICMP_REDIR_HOSTTOS, "Redirect Type of Service and Host" },
	{ ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, "Time to live exceeded" },
	{ ICMP_TIME_EXCEEDED, ICMP_EXC_FRAGTIME, "Frag reassembly time exceeded" },
};

uint32_t	icmp_specific_descriptions_size = sizeof(icmp_specific_descriptions) / sizeof(*icmp_specific_descriptions);

void	print_specific_icmp_code(int type, int code) {
	for (uint32_t i = 0; i < icmp_specific_descriptions_size; i++) {
		if (type == icmp_specific_descriptions[i].type && code == icmp_specific_descriptions[i].code) {
			printf("%s\n", icmp_specific_descriptions[i].description);
		}
	}
}

struct {
	int	type;
	char	*description;
} icmp_descriptions[] = {
	{ ICMP_ECHOREPLY, "Echo Reply" },
	{ ICMP_DEST_UNREACH, NULL },       
	{ ICMP_SOURCE_QUENCH, "Source Quench" },      
	{ ICMP_REDIRECT, NULL },           
	{ ICMP_ECHO, "Echo Request" },               
	{ ICMP_TIME_EXCEEDED, "Time exceeded" },      
	{ ICMP_PARAMETERPROB, "Parameter Prob" },      
	{ ICMP_TIMESTAMP, "Timestamp" },          
	{ ICMP_TIMESTAMPREPLY, "Timestamp Reply" },
	{ ICMP_INFO_REQUEST, "Info Request" },       
	{ ICMP_INFO_REPLY, "Info Reply" },         
	{ ICMP_ADDRESS, "Address Request" },            
	{ ICMP_ADDRESSREPLY, "Address Mask reply" },       
};

uint32_t	icmp_descriptions_size = sizeof(icmp_descriptions) / sizeof(*icmp_descriptions);

void	receive_error_message() {
	struct msghdr		msg;
	struct sockaddr_in	source_address;
	int			flags = MSG_DONTWAIT | MSG_ERRQUEUE;

	uint8_t			msg_buffer[4096]; // Way too big buffer for receving the ECHO_REPLY...
	uint8_t			control_buffer[20480]; // net.core.optmem_max

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

	ft_bzero(msg_buffer, sizeof(msg_buffer));
	ft_bzero(control_buffer, sizeof(control_buffer));
		

	ssize_t	ret = recvmsg(g_socket_fd, &msg, flags);

	if (-1 >= ret) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) { // Socket hasn't received packet.
			return ;
		}
		/* dprintf(2, "Silently ignoring error...\n"); */
		return ;

	}

	uint8_t source_address_hostname_str[NI_MAXHOST];
	uint8_t	source_address_raw_str[INET_ADDRSTRLEN];
		
	inet_ntop(AF_INET, &source_address.sin_addr,
		  (char *)source_address_raw_str, sizeof(source_address_raw_str));
	getnameinfo((const struct sockaddr *)&source_address, sizeof(source_address),
		    (char *)source_address_hostname_str, sizeof(source_address_hostname_str),
		    NULL, 0, 0);
	
	if (ret < ICMP_MINLEN) {
		dprintf(2, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
		return ;
	}

	struct icmphdr	header;

	header = *((struct icmphdr*)msg_buffer);
	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVERR) {
			struct sock_extended_err err;

			ft_memcpy(&err, CMSG_DATA(cmsg), sizeof(err));

			if (err.ee_origin == SO_EE_ORIGIN_ICMP) {

				struct sockaddr_in *offender = (struct sockaddr_in*)SO_EE_OFFENDER((struct sock_extended_err*)CMSG_DATA(cmsg));

				inet_ntop(AF_INET, &offender->sin_addr,
					  (char *)source_address_raw_str, sizeof(source_address_raw_str));
				getnameinfo((const struct sockaddr *)offender, sizeof(source_address),
					    (char *)source_address_hostname_str, sizeof(source_address_hostname_str),
					    NULL, 0, 0);
					
				if (argument_address_type == RAW_ADDRESS)
					printf("From %s icmp_seq=%d ", source_address_raw_str, header.un.echo.sequence);
				else
					printf("From %s (%s) icmp_seq=%d ", source_address_hostname_str, source_address_raw_str, header.un.echo.sequence);

				for (uint32_t i = 0; i < icmp_descriptions_size; i++) {
					if (err.ee_type == icmp_descriptions[i].type) {
						if (icmp_descriptions[i].description)
							printf("%s\n", icmp_descriptions[i].description);
						else
							print_specific_icmp_code(err.ee_type, err.ee_code);
					}
				}
				error_count++;
			}
		}
	}
}

bool	is_our_icmp_response(struct icmphdr header) {
	return header.un.echo.id == identity; // TODO: Actually we should add check for origin dst and src since one might use the same ident
}



// flags = MSG_DONTWAIT
// flags = MSG_DONTWAIT | MSG_ERRQUEUE

bool	get_original_headers(struct icmphdr *header, uint32_t bytes_available,
			     struct icmphdr **original_icmp, struct ip **original_ip) {
	if (bytes_available < ICMP_MINLEN)
		goto no_icmp;

	if (bytes_available < sizeof (struct icmp))
		goto no_original_ip;

	struct icmp	*icmp = (struct icmp*)header;
	
	uint32_t	bytes_left = bytes_available - sizeof(struct icmp);
	
	*original_ip = &icmp->icmp_ip;

	uint32_t	ip_hdr_len = (*original_ip)->ip_hl << 2;
	bytes_left		   -= ip_hdr_len;
	
	
	if (bytes_left < sizeof (struct icmphdr))
		goto no_original_icmp;

	*original_icmp = (struct icmphdr*)(((uint8_t*)(*original_ip)) + ip_hdr_len);

	
	return true;
no_icmp:
no_original_ip:
	*original_ip = NULL;
no_original_icmp:
	*original_icmp = NULL;
	return false;
}

void	receive_response() {
	assert(raw_socket);
	struct msghdr		msg;
	struct sockaddr_in	source_address;
	int			flags = MSG_DONTWAIT;

	uint8_t			msg_buffer[4096]; // Way too big buffer for receving the ECHO_REPLY...
	uint8_t			control_buffer[20480]; // net.core.optmem_max

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

	ft_bzero(msg_buffer, sizeof(msg_buffer));
	ft_bzero(control_buffer, sizeof(control_buffer));
		

	ssize_t	ret = recvmsg(g_socket_fd, &msg, flags);

	if (-1 >= ret) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) { // Socket hasn't received packet yet.
			return ;
		}
		dprintf(2, "Silently ignoring error...\n");
		return ;

	}

	size_t	packet_size_left = (size_t)ret;

	uint8_t source_address_hostname_str[NI_MAXHOST];
	uint8_t	source_address_raw_str[INET_ADDRSTRLEN];

	ft_bzero(source_address_hostname_str, sizeof(source_address_hostname_str));
	ft_bzero(source_address_raw_str, sizeof(source_address_raw_str));
	inet_ntop(AF_INET, &source_address.sin_addr,
		  (char *)source_address_raw_str, sizeof(source_address_raw_str));
	getnameinfo((const struct sockaddr *)&source_address, sizeof(source_address),
		    (char *)source_address_hostname_str, sizeof(source_address_hostname_str),
		    NULL, 0, 0);

	struct icmphdr	*icmp_header;
	struct ip	ip_header;

	uint8_t	*cursor	= msg_buffer;

	uint32_t	ip_header_len = 0;

	if (packet_size_left < sizeof(struct ip)) {
		dprintf(2, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
		return ;
	}
	
	ip_header = *(struct ip*)(cursor);
	ip_header_len = ip_header.ip_hl << 2; // the size is given in qwords (uint32_t)
	cursor += ip_header_len;
	packet_size_left -= ip_header_len;

	if (packet_size_left < ICMP_MINLEN) {
		dprintf(2, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
		return ;
	}

	icmp_header = (struct icmphdr*)(cursor);
	
	struct ip	 *original_ip_header = NULL;
	struct icmphdr	 *original_icmp_header = NULL;



	switch (icmp_header->type) {
	case ICMP_ECHOREPLY:
		original_icmp_header = icmp_header;
		break;
	case ICMP_ECHO:
	case ICMP_TIMESTAMP:
	case ICMP_TIMESTAMPREPLY:
	case ICMP_ADDRESS:
	case ICMP_ADDRESSREPLY:
		return ;  // For now we silently drop packet where there is no orginal ip header and therefor original icmp header
	default:
		if (!get_original_headers(icmp_header, packet_size_left, &original_icmp_header, &original_ip_header)) {
			dprintf(2, "packet too short (%ld bytes) from %s\n", ret, source_address_raw_str);
			return ;
		}
		break;
	}


	cursor += sizeof(struct icmphdr);
	packet_size_left -= sizeof(struct icmphdr);


	if (!is_our_icmp_response(*original_icmp_header)) { // We silently drop icmp packet which are not responses to our original ECHO packets
		/* dprintf(2, "Silently dropping foreign icmp packet\n"); */
		return ;
	}

	int	packet_ttl = ip_header.ip_ttl;
	

	if (argument_address_type == RAW_ADDRESS) {
		printf("%ld bytes from %s: ", ret, source_address_raw_str);
	} else {
		printf("%ld bytes from %s (%s): ", ret, source_address_hostname_str, source_address_raw_str);
	}

	if (icmp_header->type == ICMP_ECHOREPLY) {
		bool		round_trip_available = false;
		struct timeval	time_then;
		double		diff_ms;
		
		if (packet_size_left >= sizeof(struct timeval)) {
			time_then = *(struct timeval *)(cursor);
			struct timeval	now;

			cursor += sizeof(struct timeval);
			gettimeofday(&now, NULL);

			double	time_then_ms = timeval_to_double_ms(time_then);
			double	now_ms	     = timeval_to_double_ms(now);
			diff_ms		     = now_ms - time_then_ms;

			if (diff_ms <= min)
				min = diff_ms;
			if (diff_ms >= max)
				max = diff_ms;
			sum += diff_ms;
			sum_of_squares += diff_ms * diff_ms;

			round_trip_available = true;
		}
		char	*payload = (char*)msg_buffer + sizeof(struct icmphdr) + sizeof(struct timeval);
		(void)payload; // for debugging

		char	*extra = "";

		if (current_sequence_already_responded) {
			duplicate_count++;

			if (!suppress_dup_packet_reporting)
				extra = " (DUP!)";
		} else {
			packets_received++;
			current_sequence_already_responded = true;
		}

		if (round_trip_available) {
			printf("icmp_seq=%u ttl=%u time=%3.3lf ms%s\n", icmp_header->un.echo.sequence, packet_ttl, diff_ms, extra);
		} else {
			printf("icmp_seq=%u ttl=%u ms%s\n", icmp_header->un.echo.sequence, packet_ttl, extra);
		}
	} else {

		for (uint32_t i = 0; i < icmp_descriptions_size; i++) {
			if (icmp_header->type == icmp_descriptions[i].type) {
				if (icmp_descriptions[i].description)
					printf("%s\n", icmp_descriptions[i].description);
				else
					print_specific_icmp_code(icmp_header->type, icmp_header->code);
			}
		} // TODO: what if nothing matches? 
		if (verbose) 
			print_ip_header(original_ip_header);
	}
}

void	statistics(void) {
	struct timeval end;

	gettimeofday(&end, NULL);

	double time = timeval_to_double_ms(end) - timeval_to_double_ms(start);

	double packet_loss = (1.0 - (double)packets_received / (double)packets_sent) * 100;


	double avg = sum / (double)packets_received;
	double mdev;

	mdev = sqrt(sum_of_squares / packets_received - avg * avg); // We use the identity of standard deviation "For a finite population with equal probabilities at all points"

	char	optional_dup[64];

	optional_dup[0] = '\0';
	if (duplicate_count > 0)
		snprintf(optional_dup, sizeof(optional_dup), ", +%u duplicates", duplicate_count);
	
	char	optional_error[64];


	optional_error[0] = '\0';
	if (error_count > 0)
		snprintf(optional_error, sizeof(optional_error), ", +%u errors", error_count);

	printf("\n--- %s ping statistics ---\n", pinged_address);
	printf("%lu packets transmitted, %lu packets received%s%s, %.0lf%% packet loss, time %.0lfms\n", packets_sent, packets_received, optional_dup, optional_error, packet_loss, time);

	if (packets_received != 0) 
		printf("rtt min/avg/max/mdev = %.3lf/%.3lf/%.3lf/%.3lf ms\n", min, avg, max, mdev);
}


void	end(int signum) {
	(void)signum;

	statistics();
	exit(EXIT_SUCCESS);
}
