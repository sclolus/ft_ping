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

# include <sys/time.h>

# include <assert.h>

# include "libft.h"


# define STATIC_PAYLOAD "ft_ping#"
# define DEFAULT_PAYLOAD_SIZE 56

# define MAX(a, b) (a <= b ? b : a)

#define USAGE "Usage\n\
  ft_ping [options] <destination>\n\
Options:\n\
  -v                 verbose output\n\
  -h                 print help and exit\n"


void	receive_echo_reply();
void	receive_error_message();
void	end(int signum);
#endif
