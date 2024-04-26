#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>

#include "c2tool.h"

#define GPIO_BASE_FILE "/sys/class/gpio/gpio"

static int cmd_size;

extern struct cmd __start___cmd;
extern struct cmd __stop___cmd;

#define for_each_cmd(_cmd)					\
	for (_cmd = &__start___cmd; _cmd < &__stop___cmd;		\
	     _cmd = (const struct cmd *)((char *)_cmd + cmd_size))


static void __usage_cmd(const struct cmd *cmd, char *indent, bool full)
{
	const char *start, *lend, *end;

	printf("%s", indent);

	printf("%s", cmd->name);

	if (cmd->args) {
		/* print line by line */
		start = cmd->args;
		end = strchr(start, '\0');
		printf(" ");
		do {
			lend = strchr(start, '\n');
			if (!lend)
				lend = end;
			if (start != cmd->args) {
				printf("\t");
				printf("%s ", cmd->name);
			}
			printf("%.*s\n", (int)(lend - start), start);
			start = lend + 1;
		} while (end != lend);
	} else
		printf("\n");

	if (!full || !cmd->help)
		return;

	/* hack */
	if (strlen(indent))
		indent = "\t\t";
	else
		printf("\n");

	/* print line by line */
	start = cmd->help;
	end = strchr(start, '\0');
	do {
		lend = strchr(start, '\n');
		if (!lend)
			lend = end;
		printf("%s", indent);
		printf("%.*s\n", (int)(lend - start), start);
		start = lend + 1;
	} while (end != lend);

	printf("\n");
}

static const char *argv0;

static void usage(int argc, char **argv)
{
	const struct cmd *cmd;
	bool full = argc >= 0;
	const char *cmd_filt = NULL;

	if (argc > 0)
		cmd_filt = argv[0];

	printf("Usage:\t%s [options] <tty device> command\n", argv0);
	printf("\t--version\tshow version (%s)\n", c2tool_version);
	printf("Commands:\n");
	for_each_cmd(cmd) {
		if (!cmd->handler || cmd->hidden)
			continue;
		if (cmd_filt && strcmp(cmd->name, cmd_filt))
			continue;
		__usage_cmd(cmd, "\t", full);
	}
}

static void usage_cmd(const struct cmd *cmd)
{
	printf("Usage:\t%s [options] ", argv0);
	__usage_cmd(cmd, "", true);
}

static void version(void)
{
	printf("c2tool version %s\n", c2tool_version);
}

static int __handle_cmd(struct c2tool_state *state, int argc, char **argv,
			const struct cmd **cmdout)
{
	const struct cmd *cmd, *match = NULL;
	const char *command;

	if (argc <= 0)
		return 1;

	if (argc > 0) {
		command = *argv;

		for_each_cmd(cmd) {
			if (!cmd->handler)
				continue;
			if (strcmp(cmd->name, command))
				continue;
			if (argc > 1 && !cmd->args)
				continue;
			match = cmd;
			break;
		}

		if (match) {
			argc--;
			argv++;
		}
	}

	if (match)
		cmd = match;
	else
		return 1;

	if (cmdout)
		*cmdout = cmd;

	return cmd->handler(state, argc, argv);
}

int handle_cmd(struct c2tool_state *state, int argc, char **argv)
{
	return __handle_cmd(state, argc, argv, NULL);
}

static int init_gpio(const char* arg)
{
	int gpio;
	int fd;
	char b[256];
	char *end;

	gpio = strtol(arg, &end, 10);
	if (*end)
		return -1;

	snprintf(b, sizeof(b), "%s%d/value", GPIO_BASE_FILE, gpio);
	fd = open(b, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Open %s: %s\n", b, strerror(errno));
		return -1;
	}

	return fd;
}

#define BAUDRATE B115200
static int init_tty(const char* arg)
{
    struct termios tio;
    struct termios stdio;
    int tty_fd_l;
	unsigned char buf[200] = {0};

    memset(&stdio,0,sizeof(stdio));
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;
    if((tty_fd_l = open(arg , O_RDWR | O_NONBLOCK)) == -1){
        printf("Error while opening %s\n", arg); // Just if you want user interface error control
        exit(EXIT_FAILURE);
    }
    cfsetospeed(&tio,BAUDRATE);
    cfsetispeed(&tio,BAUDRATE);            // baudrate is declarated above
    tcsetattr(tty_fd_l,TCSANOW,&tio);
	sleep(2);
	write(tty_fd_l, "r", 1);
    read(tty_fd_l, buf, 200);
    //printf("Programmer info: %s\n", buf);
    return tty_fd_l;
}

HIDDEN(dummy1, NULL, NULL);
HIDDEN(dummy2, NULL, NULL);

int main(int argc, char **argv)
{
	int err;
	const struct cmd *cmd = NULL;
	struct c2_device_info info;
	struct c2tool_state state = {{0}};

	/* calculate command size including padding */
	cmd_size = abs((long)&__cmd_dummy1 - (long)&__cmd_dummy2);
	/* strip off self */
	argc--;
	argv0 = *argv++;

	if (argc > 0 && strcmp(*argv, "--version") == 0) {
		version();
		return 0;
	}

	if (argc < 2) {
		usage(0, NULL);
		return 1;
	}

	state.c2if.tty_fd = init_tty(*argv); // read the board after starting it
	if ( !state.c2if.tty_fd < 0 ) {
		usage(0, NULL);
		return 1;
	}
	argc--;
	argv++;


	if (c2_halt(&state.c2if) < 0)
		return 1;

	if (c2_get_device_info(&state.c2if, &info) < 0) {
		return 1;
	}

	if (c2family_find(info.device_id, &state.family)) {
		fprintf(stderr, "device family 0x%02x not supported yet\n", info.device_id);
		return 1;
	}

	err = __handle_cmd(&state, argc, argv, &cmd);

	if (err == 1) {
		if (cmd)
			usage_cmd(cmd);
		else
			usage(0, NULL);
	} else if (err < 0) {
		fprintf(stderr, "command failed: %s (%d)\n", strerror(-err), err);
	}

	if (state.c2if.tty_fd) {
		close(state.c2if.tty_fd);
	}

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
