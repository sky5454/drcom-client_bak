#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>

#include <stdio.h>
#include <string.h>

#include "tcptrack.h"

#define BUFFSIZE	1024

#define IP_DST		"172.16.0.2"
#define IP_SRC		"172.16.0.1"
#define PKT_SRC		"172.16.1.2"
#define PKT_DST		"224.0.1.2"
#define INNER_IP	"172.16.1.1"
#define EXTRA_IP	"172.16.2.1"
#define REMOTE_IP_1	"172.16.2.5"
#define REMOTE_IP_2	"172.16.2.6"
#define REMOTE_IP_3	"224.0.3.4"
#define PORT_SRC	2008
#define PORT_DST	2046

int main(void) 
{ 
	fd_set rfds, fds;
	struct timeval tv, t;
	const int TTL=64; 
	char buffer[BUFFSIZE]; 
	const int v_off=0, v_on=1; 
	struct sockaddr_in to,from; 
	struct ip_mreq multiaddr; 
	socklen_t addrlen; 
	int sock; 
	int ret;
	int idx;

	printf("socket......\n");
	sock = socket(AF_INET, SOCK_DGRAM, 0); 
	if(sock < 0)
		perror("socket");

	struct conn_param *cp = (struct conn_param*)buffer;

	memset(cp, 0, sizeof(*cp));

	strcpy(cp->devname, "ppp0");
	cp->e_count = 1;
	cp->es[0].addr = 0;
	cp->es[0].mask = 0;

	printf("setsockopt(CONN_SO_SET_PARAMS)......\n");

	ret = setsockopt(sock, IPPROTO_IP, CONN_SO_SET_PARAMS, cp, sizeof(struct conn_param)+sizeof(struct e_address));
	if (ret != 0) {
		printf("setsockopt(CONN_SO_SET_PARAMS) failed\n");
		return -1;
	}


	struct conn_auth_cmd cmd;
	cmd.cmd = CONN_MODE_AUTH;

	printf("setsockopt(CONN_SO_SET_AUTH_CMD)......");

	ret = setsockopt(sock, IPPROTO_IP, CONN_SO_SET_AUTH_CMD, &cmd, sizeof(struct conn_auth_cmd));
	if (ret != 0) {
		printf("failed\n");
		return -1;
	}

	printf("OK\n");

	close(sock); 
}

