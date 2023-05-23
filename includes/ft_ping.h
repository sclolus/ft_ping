#ifndef __FT_PING_H__
# define __FT_PING_H_

# include <stdio.h>
# include <arpa/inet.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netdb.h>
# include <netinet/ip_icmp.h>
# include <signal.h>

# include <sys/time.h>

# include <assert.h>

# include "libft.h"

# define STATIC_PAYLOAD "ft_ping#"

void	receive_echo_reply(int signum);
void	end(int signum);
#endif
