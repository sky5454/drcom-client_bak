#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"
#include "client_daemon.h"
#include "daemon.h"

#include "log.h"

static void usage(void)
{
	puts("drcomd, daemon part of the drcomc-drcomcd client-daemon programs\n\n"
		 "	usage: drcomd [ -n | --nodaemon ]\n");

	exit(EXIT_FAILURE);
}

static void daemonize(void)
{
	pid_t pid, sid;
	int fd;

	pid = fork();
	if(pid > 0)
		exit(0);
	if(pid < 0){
		logerr("fork of daemon failed: %s", strerror(errno));
		exit(-1);
	}

	fd = open("/dev/null", O_RDWR);
	if (fd >= 0) {
		if (fd != STDIN_FILENO)
			dup2(fd, STDIN_FILENO);
		if (fd != STDOUT_FILENO)
			dup2(fd, STDOUT_FILENO);
		if (fd != STDERR_FILENO)
			dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			close(fd);
	}
	if (fd < 0)
		logerr("fatal, could not open /dev/null: %s", strerror(errno));

	chdir("/");
	umask(022);

	/* become session leader */
	sid = setsid();
	dbg("our session is %d", sid);
}

static void init_module(void)
{
	int r;
	char s[50];

	strcpy(s, "/sbin/modprobe drcom");
	r = system(s);
	if (r) {
		fprintf(stderr, "drcomd: Error loading drcom module\n");
		exit(EXIT_FAILURE);
	}
}

static int init_socket(void)
{
	int s, r;
	struct sockaddr_un un_daemon;

	memset(&un_daemon, 0x00, sizeof(struct sockaddr_un));
	un_daemon.sun_family = AF_UNIX;
	/* use abstract namespave */
	strncpy(&un_daemon.sun_path[1], DRCOMCD_SOCK, sizeof(un_daemon.sun_path)-1);

	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s == -1) {
		fprintf(stderr, "drcomd: Socket creation failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	/* this ensures only one copy running */
	r = bind(s, (struct sockaddr *) &un_daemon, sizeof(un_daemon));
	if (r) {
		fprintf(stderr, "drcomd: Bind failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	r = listen(s, 1);
	if (r) {
		fprintf(stderr, "drcomd: Listen failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return s;
}

int main(int argc, char **argv)
{
	int s;
	int daemon = 1;
	int i;

	if(argc > 2)
		usage();

	for (i = 1 ; i < argc; i++) {
		char *arg = argv[i];
		if (strcmp(arg, "--nodaemon") == 0 || strcmp(arg, "-n") == 0) {
			printf("%s: log to stderr.\n", argv[0]);
			daemon = 0;
		}
	}

	init_module();
	s = init_socket();

	logging_init("drcomd", daemon);

	if (daemon)
		daemonize();

	drcomcd_daemon(s);

	logging_close();
	return 0;
}

