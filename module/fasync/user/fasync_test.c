#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <linux/input.h>

static int fd;

void my_signal_fun(int signum, siginfo_t *siginfo, void *act)
{
	int ret;
	char buf[64];

	if (signum == SIGIO) {
		if (siginfo->si_band & POLLIN) {
			printf("FIFO is not empty\n");
			if ((ret = read(fd, buf, sizeof(buf))) != -1) {
				buf[ret] = '\0';
				puts(buf);
			}
		}
		if (siginfo->si_band & POLLOUT)
			printf("FIFO is not full\n");
	}
}

int main(int argc, char *argv[])
{
	int ret;
	int flag;
	struct sigaction act, oldact;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGIO);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = my_signal_fun;
	if (sigaction(SIGIO, &act, &oldact) == -1)
		goto fail;

	fd = open("/dev/my_demo_dev0", O_RDWR);
	if (fd < 0)
		goto fail;

	/* Set async io own */
	if (fcntl(fd, F_SETOWN, getpid()) == -1)
		goto fail;

	/* Set SIGIO signal */
	if (fcntl(fd, F_SETSIG, SIGIO) == -1)
		goto fail;

	/* Get file flags */
	if ((flag = fcntl(fd, F_GETFL)) == -1)
		goto fail;

	/* Set file flags, and FASYNC */
	if (fcntl(fd, F_SETFL, flag | FASYNC) == -1)
		goto fail;

	while (1)
		sleep(1);

fail:
	perror("fasync test");
	exit(EXIT_FAILURE);

	return 0;
}