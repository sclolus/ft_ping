#ifndef __FT_PING_H__
# define __FT_PING_H_

# include <stdio.h>
# include <arpa/inet.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netdb.h>
# include <netinet/ip_icmp.h>
# include <netinet/ip.h>
# include <linux/errqueue.h>
# include <signal.h>
# include <math.h>
# include <stdbool.h>
# include <sys/time.h>

# include <assert.h>

# include "libft.h"


# define STATIC_PAYLOAD "ft_ping#"
# define DEFAULT_PAYLOAD_SIZE 56

# define MAX(a, b) (a <= b ? b : a)
# define ITEM_COUNT(t) (sizeof(t) / sizeof(*t))

# define USAGE "Usage\n\
  ft_ping [options] <destination>\n\
Options:\n\
  -v                 verbose output\n\
  -h                 print help and exit\n\
For more details see ping(8)\n"

# define ICMP_UNKNOWN_TYPE NR_ICMP_TYPES
# define ICMP_UNKNOWN_CODE ((uint8_t)-1)
# define ICMP_DEFAULT_CODE 0

void		receive_echo_reply();
void		receive_response();
void		receive_error_message();
void		end(int signum);
uint16_t	icmp_checksum(void *data, uint32_t len);
void		print_icmp_header(struct icmphdr *icmp, uint32_t size);
void		print_ip_header(struct ip *ip);
uint16_t	ft_ntohs(uint16_t netshort);
#endif
