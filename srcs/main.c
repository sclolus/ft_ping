#include "ft_ping.h"
#include <stdbool.h>

#define USAGE "ft_ping: [-v] [-h] <destination>\n"

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
	
	dprintf(2, "%s: %s\n", header, err);
}

static double	timeval_to_double_ms(struct timeval time) {
	return (double)time.tv_sec * 1000 // A second is 1000 ms....
		+ (double)time.tv_usec / 1000; // a micro-second is 1/1000th of a ms....
}


int		g_socket_fd;
uint32_t	ttl = 64;
uint32_t	sequence = 1;
uint32_t	duplicate_count = 0;
uint32_t	error_count = 0;
bool		current_sequence_already_responded = false;
bool		suppress_dup_packet_reporting = false;


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

int main(int argc, char **argv) {

	(void)argc;
	(void)argv;

	if (argc != 2) {
		dprintf(2, USAGE);
		exit(EXIT_FAILURE);
	}

	char *argument_address = argv[1];

	pinged_address = argument_address;

	uint8_t	resolved_address[INET_ADDRSTRLEN];

	struct in_addr	   internet_address;
	struct sockaddr_in socket_address;

	int res;
	struct addrinfo *matched_addresses = NULL;

	res = inet_pton(AF_INET, argument_address, &internet_address);
	
	if (res == 0) {
		struct addrinfo hints = {
			.ai_family = AF_INET,
			.ai_socktype = SOCK_DGRAM,
			/* .ai_protocol = IPPROTO_ICMP, */
			.ai_protocol = 0,
			.ai_flags = AI_CANONNAME,
		};
		
		int failure = getaddrinfo(argument_address, NULL, &hints, &matched_addresses);

		if (failure) {
			const char *error_string = gai_strerror(failure);
			
			dprintf(2, "ping: %s: %s\n", argument_address, error_string);
			exit(EXIT_FAILURE);
		}

		socket_address = *((struct sockaddr_in*)matched_addresses->ai_addr); //Should be struct sockaddr_in
		dprintf(2, "%s! proto: %d\n", matched_addresses->ai_canonname, matched_addresses->ai_protocol);
	} else {
		dprintf(2, "No getaddrinfo was called as it is unnecessary\n");
			
		socket_address.sin_addr = internet_address;
		socket_address.sin_family = AF_INET;
		socket_address.sin_port = 0; // Should not matter for icmp
	}

	inet_ntop(AF_INET, &socket_address.sin_addr, (char*)resolved_address, sizeof(resolved_address));

	int socket_fd;

	socket_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_ICMP);

	if (-1 == socket_fd) {
		ft_perror("unprivileged socket call failed");
		exit(EXIT_FAILURE);
	}
	dprintf(2, "Socket DGRAM for unpriviliged ICMP has been initialized\n");

	if (-1 == set_socket_options(socket_fd)) {
		ft_perror("set_socket_options");
		exit(EXIT_FAILURE);
	}

	g_socket_fd = socket_fd;
	signal(SIGINT, end);
	

	dprintf(2, "getpid() == %u\n", getpid());

	uint8_t		icmp_message[sizeof (struct icmphdr ) + sizeof(struct timeval) + sizeof (STATIC_PAYLOAD) + 10];


	printf("PING %s (%s) %lu(%lu) bytes of data.\n", argument_address, resolved_address, sizeof(icmp_message) - sizeof(struct icmphdr), sizeof(icmp_message) + 20); // + 20 for the size of the ipv4 hdr

	struct timeval prev;	
	gettimeofday(&start, NULL);

	prev = start;
	for (;;) {
		struct timeval	now;
		gettimeofday(&now, NULL);

		double		ms_diff = timeval_to_double_ms(now) - timeval_to_double_ms(prev);


		if (ms_diff >= 1000) { // we're due to send a packet.
		
			int		flags = MSG_DONTWAIT;
			struct icmphdr	header;

			header.type = ICMP_ECHO;
			header.code = 0; // Code must be zero
			header.un.echo.id = 0; // That or ICMP port? 
			header.un.echo.sequence = sequence;

			pid_t		pid = getpid();
			char		*spid = ft_static_ulltoa(pid);

			ft_bzero(icmp_message, sizeof(icmp_message));
			ft_memcpy(icmp_message, &header, sizeof(struct icmphdr));
			ft_memcpy(icmp_message + sizeof(struct icmphdr), &now, sizeof (struct timeval));
			ft_memcpy(icmp_message + sizeof(struct icmphdr) + sizeof (struct timeval), STATIC_PAYLOAD, sizeof (STATIC_PAYLOAD));
			ft_strcpy((char *)(icmp_message + sizeof(struct icmphdr) + sizeof(struct timeval) + sizeof (STATIC_PAYLOAD) - 1), spid);

			int bytes = sendto(socket_fd,
					   icmp_message,
					   sizeof(icmp_message),
					   flags,
					   (struct sockaddr*)&socket_address,
					   sizeof(struct sockaddr_in));

			bool failure = bytes == -1;
			
			current_sequence_already_responded = false;
			if (failure) {
				ft_perror("Sendto(): ");
				exit(EXIT_FAILURE);			
			}
			packets_sent++;
			sequence++;
			gettimeofday(&prev, NULL);
			dprintf(2, "Send packet\n");
		}
		receive_echo_reply();
		receive_error_message();
		usleep(20);
	}

	return 0;
}

void	receive_echo_reply() {
	struct msghdr		msg;
	struct sockaddr_in	source_address;
	int			flags = MSG_DONTWAIT;

	uint8_t			msg_buffer[4096]; // Way too big buffer for receving the ECHO_REPLY...
	uint8_t			control_buffer[4096];

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


	if (-1 == ret) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) { // Socket hasn't received packet.
			return ;
		}
		
		dprintf(2, "Silently ignoring error...\n");
		return ;
	}


	dprintf(2, "flags: %u\n", msg.msg_flags);
	dprintf(2, "--- flags ---\n");
	for (char const **flag = recvmsg_flags_strings(msg.msg_flags); *flag != NULL; flag++) {
		dprintf(2, "Flag returned by recvmsg: %s\n", *flag);
	}
	dprintf(2, "-------------\n");

	
	struct icmphdr	header;

	header = *((struct icmphdr*)msg_buffer);

	dprintf(2, "Received msg of size: %lu \n Header type: %u\n Header code: %u\n Header echo id %u\n Header echo sequence %u\n", ret, header.type, header.code, header.un.echo.id, header.un.echo.sequence);

	
	uint8_t	source_address_str[INET_ADDRSTRLEN];
	
	inet_ntop(AF_INET, &source_address.sin_addr, (char*)source_address_str, sizeof(source_address_str));

	int	packet_ttl = ttl; // if not found resort to default ttl
	
	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		dprintf(2, "cmsg.level = %d, cmsg.type = %d\n", cmsg->cmsg_level, cmsg->cmsg_type);
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
			ft_memcpy(&packet_ttl, CMSG_DATA(cmsg), sizeof(packet_ttl));

			dprintf(2, "Found ttl: %d!\n", packet_ttl);			
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

		dprintf(2, "Payload is: %s\n", payload);

		dprintf(2, "name (of len %u): \n", msg.msg_namelen);

		char	*extra = "";

		if (current_sequence_already_responded) {
			duplicate_count++;

			if (!suppress_dup_packet_reporting)
				extra = " (DUP!)";
		} else {
			packets_received++;
			current_sequence_already_responded = true;
		}

		printf("%ld bytes from %s: icmp_seq=%u ttl=%u time=%3.3lf ms%s\n", ret, source_address_str, header.un.echo.sequence, packet_ttl, diff_ms, extra);
	} else {
		dprintf(2, "Response of unknown type received\n");
	}
}

void	receive_error_message() {
	struct msghdr		msg;
	struct sockaddr_in	source_address;
	int			flags = MSG_DONTWAIT | MSG_ERRQUEUE;

	uint8_t			msg_buffer[4096]; // Way too big buffer for receving the ECHO_REPLY...
	uint8_t			control_buffer[4096]; // TODO: how big do we need it?

	struct iovec		msg_iov = {
		.iov_base = msg_buffer,
		.iov_len  = sizeof(msg_buffer),
	};
	
	msg.msg_name = &source_address;
	msg.msg_namelen = sizeof(source_address);

	msg.msg_iov = &msg_iov;
	msg.msg_iovlen = 1;
	
	msg.msg_control = control_buffer; // I don't know if I need this
	msg.msg_controllen = sizeof(control_buffer);
	
	msg.msg_flags = 0; // Could be set after call

	ssize_t	ret = recvmsg(g_socket_fd, &msg, flags);

	if (-1 == ret) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) { // Socket hasn't received packet.
			return ;
		}
		dprintf(2, "Silently ignoring error...\n");
		return ;

	}

	struct icmphdr	header;

	header = *((struct icmphdr*)msg_buffer);

	dprintf(2, "Received msg of size: %lu \n Header type: %u\n Header code: %u\n Header echo id %u\n Header echo sequence %u\n", ret, header.type, header.code, header.un.echo.id, header.un.echo.sequence);

	
	uint8_t	source_address_str[INET_ADDRSTRLEN];
	
	inet_ntop(AF_INET, &source_address.sin_addr, (char *)source_address_str, sizeof(source_address_str));

	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		dprintf(2, "Error: cmsg.level = %d, cmsg.type = %d\n", cmsg->cmsg_level, cmsg->cmsg_type);
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVERR) {
			struct sock_extended_err err;

			ft_memcpy(&err, CMSG_DATA(cmsg), sizeof(err));

			if (err.ee_origin == SO_EE_ORIGIN_ICMP) {
				if (err.ee_type == ICMP_TIME_EXCEEDED) {
					printf("From %s (%s) icmp_seq=%d Time to live exceeded\n", source_address_str, source_address_str, header.un.echo.sequence); // TODO: Address resolution or not?
				} else if (err.ee_type == ICMP_DEST_UNREACH) {
					printf("From %s (%s) icmp_seq=%d Destination Host Unreachable\n", source_address_str, source_address_str, header.un.echo.sequence); // TODO: Address resolution or not?
				}
				error_count++;
			}
		}
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

	if (duplicate_count > 0)
		snprintf(optional_dup, sizeof(optional_dup), ", +%u duplicates", duplicate_count);
	else
		snprintf(optional_dup, sizeof(optional_dup), "");
	
	char	optional_error[64];

	if (error_count > 0)
		snprintf(optional_error, sizeof(optional_error), ", +%u errors", error_count);
	else
		snprintf(optional_error, sizeof(optional_error), "");

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
