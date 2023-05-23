#include "ft_ping.h"
#include <stdbool.h>

#define USAGE "ft_ping: [-v] [-h] <destination>\n"

static void ft_perror(char *header) {
	char *err = strerror(errno);
	
	dprintf(2, "%s: %s\n", header, err);
}

static double	timeval_to_double_ms(struct timeval time) {
	return (double)time.tv_sec * 1000 // A second is 1000 ms....
		+ (double)time.tv_usec / 1000; // a micro-second is 1/1000th of a ms....
}


int g_socket_fd;
uint32_t ttl = 64;

int	set_socket_options(int fd) {
	if (-1 == setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))) {
		ft_perror("setsockopt()");
		return -1;
	}
	return 0;
}

const char	*pinged_address;
struct timeval	start;
double		min = INFINITY;
double		max = -INFINITY;
double		sum;
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
			
			dprintf(2, "getaddrinfo failed: %s\n", error_string);
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

	inet_ntop(AF_INET, &socket_address.sin_addr, resolved_address, sizeof(resolved_address));

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


	printf("PING %s (%s) %u(%u) bytes of data.\n", argument_address, resolved_address, sizeof(icmp_message) - sizeof(struct icmphdr), sizeof(icmp_message) + 20); // + 20 for the size of the ipv4 hdr
	uint32_t	sequence = 1;

	


	struct timeval prev;	
	gettimeofday(&start, NULL);

	prev = start;
	for (;;) {
		struct timeval	now;
		gettimeofday(&now, NULL);

		struct timeval	diff;


		timersub(&now, &prev, &diff); // TODO: THIS FUNCTION IS NOT IN THE SUBJECT WE NEED TO CODE IT.
		
		if (diff.tv_sec >= 1) { // we're due to send a packet.
		
			int		flags = MSG_DONTWAIT;
			struct icmphdr	header;

			header.type = ICMP_ECHO;
			header.code = 0; // Code must be zero
			header.un.echo.id = 0; // That or ICMP port? 
			header.un.echo.sequence = sequence;

			pid_t		pid = getpid();
			char		*spid = ft_static_ulltoa(pid);
			uint32_t	spid_len = ft_strlen(spid);

			ft_bzero(icmp_message, sizeof(icmp_message));
			ft_memcpy(icmp_message, &header, sizeof(struct icmphdr));
			ft_memcpy(icmp_message + sizeof(struct icmphdr), &now, sizeof (struct timeval));
			ft_memcpy(icmp_message + sizeof(struct icmphdr) + sizeof (struct timeval), STATIC_PAYLOAD, sizeof (STATIC_PAYLOAD));
			ft_strcpy(icmp_message + sizeof(struct icmphdr) + sizeof(struct timeval) + sizeof (STATIC_PAYLOAD) - 1, spid);


		
			int bytes = sendto(socket_fd,
					   icmp_message,
					   sizeof(icmp_message),
					   flags,
					   (struct sockaddr*)&socket_address,
					   sizeof(struct sockaddr_in));

			bool failure = bytes == -1;

			if (failure) {
				ft_perror("Sendto(): ");
				exit(EXIT_FAILURE);			
			}
			packets_sent++;
			sequence++;
			gettimeofday(&prev, NULL);
			dprintf(2, "Send packet\n");
		}
		receive_echo_reply(0);
		usleep(20);
	}

	return 0;
}

void	receive_echo_reply(int signum) {
	struct msghdr		msg;
	struct sockaddr_in	source_address;
	int			flags = MSG_DONTWAIT;

	uint8_t			msg_buffer[4096]; // Way too big buffer for receving the ECHO_REPLY... 

	struct iovec		msg_iov = {
		.iov_base = msg_buffer,
		.iov_len  = sizeof(msg_buffer),
	};
	
	msg.msg_name = &source_address;
	msg.msg_namelen = sizeof(source_address);

	msg.msg_iov = &msg_iov;
	msg.msg_iovlen = 1;
	
	msg.msg_control = NULL; // I don't know if I need this
	msg.msg_controllen = 0;
	
	msg.msg_flags = 0; // Could be set after call

	ssize_t	ret = recvmsg(g_socket_fd, &msg, flags);

	if (-1 == ret) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) { // Socket hasn't received packet.
			return ;
		}
		
		ft_perror("recvmsg(): ");
		exit(EXIT_FAILURE);
	}

	packets_received++;
	
	struct icmphdr	header;

	header = *((struct icmphdr*)msg_buffer);

	dprintf(2, "Received msg of size: %lu \n Header type: %u\n Header code: %u\n Header echo id %u\n Header echo sequence %u\n", ret, header.type, header.code, header.un.echo.id, header.un.echo.sequence);

	struct timeval	time_then = *(struct timeval *)(msg_buffer + sizeof(struct icmphdr));
	struct timeval	now;
	struct timeval	diff;	

	gettimeofday(&now, NULL);
	
	timersub(&now, &time_then, &diff); //TODO: THIS FUNCTION IS NOT IN THE SUBJECT WE DID TO CODE IT.

	double	time_then_ms = timeval_to_double_ms(time_then);
	double	now_ms	     = timeval_to_double_ms(now);
	double	diff_ms	     = now_ms - time_then_ms;

	if (diff_ms <= min)
		min = diff_ms;
	if (diff_ms >= max);
		max = diff_ms;
		
	sum += diff_ms;
		
	

	char	*payload = msg_buffer + sizeof(struct icmphdr) + sizeof(struct timeval);

	dprintf(2, "Payload is: %s\n", payload);

	dprintf(2, "name (of len %u): \n", msg.msg_namelen);

	uint8_t	source_address_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &source_address.sin_addr, source_address_str, sizeof(source_address_str));

	printf("%llu bytes from %s: icmp_seq=%u ttl=%u time=%3.3lf ms\n", ret, source_address_str, header.un.echo.sequence, ttl, diff_ms);
}

void	statistics(void) {
	struct timeval end;

	gettimeofday(&end, NULL);

	// calculate the number of ms from start to end.

	uint64_t	time;


	double packet_loss = (1.0 - (double)packets_received / (double)packets_sent) * 100;


	double avg = sum / (double)packets_received;
	double mdev;


	printf("--- %s ping statistics ---\n", pinged_address);
	printf("%llu packets transmitted, %llu packets received, %.0lf%% packet loss, time %llums\n", packets_sent, packets_received, packet_loss, time);
	printf("rtt min/avg/max/mdev = %.3lf/%.3lf/%.3lf/%.3lf ms\n", min, avg, max, mdev);
}


void	end(int signum) {
	(void)signum;


	statistics();
	exit(EXIT_SUCCESS);
}
