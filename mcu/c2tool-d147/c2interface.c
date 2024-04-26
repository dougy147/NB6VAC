#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "c2family.h"
#include "c2interface.h"
#include "c2tool.h"
#include "hexdump.h"
/*
 * C2 registers & commands defines
 */

/* C2 registers */
#define C2_DEVICEID		0x00
#define C2_REVID		0x01
#define C2_DEVCTL		0x02 // = FPCTL

/* added by dougy */
#define C2_EPCTL 	0xdf
#define C2_EPDAT 	0xbf
#define C2_EPSTAT 	0xb7
#define C2_EPADDRH 	0xaf
#define C2_EPADDRL 	0xae
#define C2_EPCTL_PM_STEP1 	0x40    /* PM = program mode */
#define C2_EPCTL_PM_STEP2 	0x58    /* PM = program mode */
/* ----- */

/* C2 interface commands */
#define C2_GET_VERSION	0x01
#define C2_DEVICE_ERASE	0x03
#define C2_BLOCK_READ	0x06
#define C2_BLOCK_WRITE	0x07
#define C2_PAGE_ERASE	0x08

#define C2_FPDAT_GET_VERSION	0x01
#define C2_FPDAT_GET_DERIVATIVE	0x02
#define C2_FPDAT_DEVICE_ERASE	0x03
#define C2_FPDAT_BLOCK_READ	0x06
#define C2_FPDAT_BLOCK_WRITE	0x07
#define C2_FPDAT_PAGE_ERASE	0x08
#define C2_FPDAT_DIRECT_READ	0x09
#define C2_FPDAT_DIRECT_WRITE	0x0a
#define C2_FPDAT_INDIRECT_READ	0x0b
#define C2_FPDAT_INDIRECT_WRITE	0x0c

#define C2_FPDAT_RETURN_INVALID_COMMAND	0x00
#define C2_FPDAT_RETURN_COMMAND_FAILED	0x02
#define C2_FPDAT_RETURN_COMMAND_OK	0x0D

#define C2_DEVCTL_HALT		0x01
#define C2_DEVCTL_RESET		0x02
#define C2_DEVCTL_CORE_RESET	0x04

#define GPIO_BASE_FILE "/sys/class/gpio/gpio"

/*
 * state 0: drive low
 *       1: high-z
 */


unsigned char set_clock_1[] = {'C'};
unsigned char set_clock_0[] = {'c'};
unsigned char set_data_1[]  = {'O'};
unsigned char set_data_0[]  = {'o'};
unsigned char get_data[]    = {'i'};
unsigned char reset_cmd[]   = {'r'};
unsigned char strobe_cmd[]  = {'s'};

unsigned char dwrite_cmd[]  = {'D'};
unsigned char dread_cmd[]   = {'d'};
unsigned char arread_cmd[]  = {'A'};

unsigned char c2d_driver_off[]  = {'v'};
unsigned char c2d_driver_on[]  = {'V'};

static void c2d_set(struct c2interface *c2if, int state)
{
	int ret;
	//printf("[C2D set] state = %d\n",state);
	if (state)
		ret = write(c2if->tty_fd, set_data_1, 1);
	else
		ret = write(c2if->tty_fd, set_data_0, 1);
}

static int c2d_get(struct c2interface *c2if)
{
	unsigned char buf[200] = {0};
	int cnt=-1, ret;

	ret = write(c2if->tty_fd, get_data , 1);
	usleep(8000);
	while (cnt == -1) {
		cnt = read(c2if->tty_fd, buf, 1);
		usleep(100);
	}
//	printf("get = %c %d\n", buf[0], cnt);
	return buf[0] == '1';
}

static void c2ck_set(struct c2interface *c2if, int state)
{
	int ret;
	if (state)
		ret = write(c2if->tty_fd, set_clock_1, 1);
	else
		ret = write(c2if->tty_fd, set_clock_0, 1);
}

static void c2ck_strobe(struct c2interface *c2if)
{
	int ret;
	ret = write(c2if->tty_fd, strobe_cmd, 1);
}

/*
 * C2 primitives
 */

void c2_reset(struct c2interface *c2if)
{
	/* To reset the device we have to keep clock line low for at least
	 * 20us.
	 */
	c2ck_set(c2if, 0);
	usleep(25);
	c2ck_set(c2if, 1);

	usleep(1);
}

static void c2_write_ar(struct c2interface *c2if, unsigned char addr)
{
	int i;

	/* START field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* INS field (11b, LSB first) */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);

	/* ADDRESS field */
	for (i = 0; i < 8; i++) {
		c2d_set(c2if, addr & 0x01);
		c2ck_strobe(c2if);

		addr >>= 1;
	}

	/* STOP field */
	c2d_set(c2if, 1);
	c2ck_strobe(c2if);
}

static int c2_read_ar(struct c2interface *c2if, unsigned char *addr)
{
	unsigned char buf[200] = {0};
	int cnt=-1, ret;

	printf("c2_read_ar\n");

	ret = write(c2if->tty_fd, arread_cmd, 1);
//	usleep(1000);
	while (cnt == -1) {
		cnt = read(c2if->tty_fd, buf, 1);
//		usleep(100);
	}
//	printf("ar_read = %x %d\n", buf[0], cnt);
	*addr = buf[0];

	return 0;

}

static int c2_write_dr(struct c2interface *c2if, unsigned char data)
{
	unsigned char buf[200] = {0};
	int cnt=-1, ret;

	ret = write(c2if->tty_fd, dwrite_cmd, 1);

	ret = write(c2if->tty_fd, &data , 1);
	//printf("&data = %x  ;  ret = %d\n", data, ret);
	usleep(5000);

	while (cnt == -1) {
		cnt = read(c2if->tty_fd, buf, 1);
		usleep(100);
	}

	if (buf[0] == '1') {
		//printf("[ERROR] c2_write_dr: buf[0] == '1' for data = %x\n", data);
		return -EIO;
	}

	//printf("c2_write_dr[%d] = %d %d\n", data, buf[0], cnt);
	return 0;

}

static int c2_read_dr(struct c2interface *c2if, unsigned char *data)
{
	unsigned char buf[200] = {0};
	int cnt=-1, ret;

	ret = write(c2if->tty_fd, dread_cmd, 1);
	printf("ret = %d", ret);
	usleep(5000);

	while (cnt == -1) {
		cnt = read(c2if->tty_fd, buf, 1);
		usleep(100);
	}

	printf("dr_read %x %d\t", buf[0], cnt);
	*data = buf[0];
	cnt = -1;
	while (cnt == -1) {
		cnt = read(c2if->tty_fd, buf, 1);
		usleep(100);
	}
	printf("dr_read = %x %d\n", buf[0], cnt);

	if (buf[0] == '1') {
		return -EIO;
	}
	return 0;
}

static int c2_poll_in_busy(struct c2interface *c2if)
{
	unsigned char addr;
	int ret, timeout = 20;

	do {
		ret = (c2_read_ar(c2if, &addr));
		if (ret < 0)
			return -EIO;

		if (!(addr & 0x02))
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	return 0;
}

static int c2_poll_out_ready(struct c2interface *c2if)
{
	unsigned char addr;
	int ret, timeout = 10000;	/* erase flash needs long time... */

	do {
		ret = (c2_read_ar(c2if, &addr));
		if (ret < 0)
			return -EIO;

		if (addr & 0x01)
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	return 0;
}

int c2_read_sfr(struct c2interface *c2if, unsigned char sfr)
{
	unsigned char data;

	c2_write_ar(c2if, sfr);

	if (c2_read_dr(c2if, &data) < 0)
		return -EIO;

	return data;
}

int c2_write_sfr(struct c2interface *c2if, unsigned char sfr, unsigned char data)
{
	c2_write_ar(c2if, sfr);

	if (c2_write_dr(c2if, data) < 0)
		return -EIO;

	return 0;
}

/*
 * Programming interface (PI)
 * Each command is executed using a sequence of reads and writes of the FPDAT register.
 */

static int c2_pi_write_command(struct c2interface *c2if, unsigned char command)
{
	if (c2_write_dr(c2if, command) < 0)
		return -EIO;

	if (c2_poll_in_busy(c2if) < 0)
		return -EIO;

	return 0;
}

static int c2_pi_get_data(struct c2interface *c2if, unsigned char *data)
{
	if (c2_poll_out_ready(c2if) < 0)
		return -EIO;

	if (c2_read_dr(c2if, data) < 0)
		return -EIO;

	return 0;
}

static int c2_pi_check_command(struct c2interface *c2if)
{
	unsigned char response;

	if (c2_pi_get_data(c2if, &response) < 0)
		return -EIO;

	if (response != C2_FPDAT_RETURN_COMMAND_OK)
		return -EIO;

	return 0;
}

static int c2_pi_command(struct c2interface *c2if, unsigned char command, int verify,
			 unsigned char *result)
{
	if (c2_pi_write_command(c2if, command) < 0)
		return -EIO;

	if (!verify)
		return 0;

	if (c2_pi_check_command(c2if) < 0)
		return -EIO;

	if (!result)
		return 0;

	if (c2_pi_get_data(c2if, result) < 0)
		return -EIO;

	return 0;
}

int c2_read_direct(struct c2tool_state *state, unsigned char reg)
{
	unsigned char data;
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DIRECT_READ, 1, NULL))
		return -EIO;

	if (c2_pi_write_command(c2if, reg))
		return -EIO;

	if (c2_pi_write_command(c2if, 0x01))
		return -EIO;
	if (c2_poll_out_ready(c2if) < 0)
		return -EIO;
	if (c2_read_dr(c2if, &data) < 0)
		return -EIO;

	return data;
}

int c2_write_direct(struct c2tool_state *state, unsigned char reg, unsigned char value)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DIRECT_WRITE, 1, NULL))
		return -EIO;

	if (c2_pi_write_command(c2if, reg))
		return -EIO;

	if (c2_pi_write_command(c2if, 0x01))
		return -EIO;

	if (c2_pi_write_command(c2if, value))
		return -EIO;

	return 0;
}

int c2_flash_read(struct c2tool_state *state, unsigned int addr, unsigned int length,
			 unsigned char *dest)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	while (length) {
		unsigned int blocksize;
		unsigned int k;

		if (c2_pi_command(c2if, C2_FPDAT_BLOCK_READ, 1, NULL) < 0)
			return -EIO;

		if (c2_pi_command(c2if, addr >> 8, 0, NULL) < 0)
			return -EIO;
		if (c2_pi_command(c2if, addr & 0xff, 0, NULL) < 0)
			return -EIO;

		if (length > 255) {
			if (c2_pi_command(c2if, 0, 1, NULL) < 0)
				return -EIO;
			blocksize = 256;
		} else {
			if (c2_pi_command(c2if, length, 1, NULL) < 0)
				return -EIO;
			blocksize = length;
		}

		for (k = 0; k < blocksize; ++k) {
			unsigned char data;

			if (c2_pi_get_data(c2if, &data) < 0)
				return -EIO;
			if (dest)
				*dest++ = data;
		}

		length -= blocksize;
		addr += blocksize;
	}

	return 0;
}

static int c2_flash_write(struct c2tool_state *state, unsigned int addr, unsigned int length,
			 unsigned char *src)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	while (length) {
		unsigned int blocksize;
		unsigned int k;

		if (c2_pi_command(c2if, C2_FPDAT_BLOCK_WRITE, 1, NULL) < 0)
			return -EIO;

		if (c2_pi_command(c2if, addr >> 8, 0, NULL) < 0)
			return -EIO;
		if (c2_pi_command(c2if, addr & 0xff, 0, NULL) < 0)
			return -EIO;

		if (length > 255) {
			if (c2_pi_command(c2if, 0, 1, NULL) < 0)
				return -EIO;
			blocksize = 256;
		} else {
			if (c2_pi_command(c2if, length, 1, NULL) < 0)
				return -EIO;
			blocksize = length;
		}

		for (k = 0; k < blocksize; ++k) {
			if (c2_pi_command(c2if, *src++, 0, NULL) < 0)
				return -EIO;
		}

		length -= blocksize;
		addr += blocksize;
	}

	return 0;
}

int c2_flash_erase(struct c2tool_state *state, unsigned char page)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_PAGE_ERASE, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, page, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0, 1, NULL) < 0)
		return -EIO;

	return 0;
}

int c2_halt(struct c2interface *c2if)
{
	/* not really halting...
	   more "enabling c2 programming" (see p.13, AN127.pdf)
	*/
	c2_reset(c2if);
	usleep(2);

	c2_write_ar(c2if, C2_DEVCTL);

	// 0x02
	if (c2_write_dr(c2if, C2_DEVCTL_RESET) < 0)  // <- blocking here
		return -EIO;

	// 0x01
	if (c2_write_dr(c2if, C2_DEVCTL_CORE_RESET) < 0)
		return -EIO;

	// 0x04
	if (c2_write_dr(c2if, C2_DEVCTL_HALT) < 0)
		return -EIO;

	//usleep(20000);

	///* added by dougy:
	//   enabling c2 program mode for EPCTL (normally only for write operation)
	//*/

	//if (c2_write_dr(c2if, C2_EPCTL_PM_STEP1) < 0)
	//	return -EIO;

	//if (c2_write_dr(c2if, C2_EPCTL_PM_STEP2) < 0) {
	//	return -EIO;
	//}

	usleep(20000);

	return 0;
}

int c2_get_device_info(struct c2interface *c2if, struct c2_device_info *info)
{
	unsigned char data;
	int ret;

	c2_write_ar(c2if, C2_DEVICEID); // C2_DEVICEID address = 0x00

	printf("starting c2_read_dr(c2if, data = %x)\n", data);
	ret = c2_read_dr(c2if, &data); // this is where it stops

	printf("ret=%d\n", ret);

	if (ret < 0) {
		return -EIO;
	}

	info->device_id = data; // here it insert harvested "data" into "device_id"
				// for some reason I get '49' while I expect '17'

	printf("data = %d\n", data);

	/* Select REVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_REVID);

	/* Read and return the revision ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0) {
		return -EIO;
	}
	printf("ret = %d\n", data);

	info->revision_id = data;

	//printf("Returning from c2_get_device_info\n");
	return 0;
}

int c2_get_pi_info(struct c2tool_state *state, struct c2_pi_info *info)
{
	unsigned char data;
	int ret;
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	/* Select FPDAT register for C2 data register accesses */
	c2_write_ar(c2if, family->fpdat);

	ret = c2_pi_command(c2if, C2_FPDAT_GET_VERSION, 1, &data);
	if (ret < 0)
		return -EIO;

	info->version = data;

	ret = c2_pi_command(c2if, C2_FPDAT_GET_DERIVATIVE, 1, &data);
	if (ret < 0)
		return -EIO;

	info->derivative = data;

	return 0;
}

int c2_flash_erase_device(struct c2tool_state *state)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DEVICE_ERASE, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0xde, 0, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0xad, 0, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0xa5, 1, NULL) < 0)
		return -EIO;

	return 0;
}

int flash_chunk(struct c2tool_state *state, unsigned int addr, unsigned int length,
		       unsigned char *src)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;
	unsigned int page_size = family->page_size;
	unsigned int page = addr / page_size;
	unsigned char buf[page_size];
	int must_read = (addr % page_size) || (length < page_size);
	unsigned int page_base = page * page_size;
	unsigned int chunk_start = addr - page_base;
	unsigned int chunk_len = (chunk_start + length > page_size) ?
				 (page_size  - chunk_start) : length;

	if (must_read) {
		if (c2_flash_read(state, page_base, page_size, buf))
			return -EIO;
	}

	memcpy(buf + addr - page_base, src, chunk_len);

	if (c2_flash_erase(state, page))
		return -EIO;

	if (c2_flash_write(state, page_base, page_size, buf))
		return -EIO;

	return chunk_len;
}
