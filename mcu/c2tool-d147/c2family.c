#include <stddef.h>
#include <errno.h>

#include "c2family.h"
#include "c2interface.h"
#include "c2tool.h"

enum {
	C2_DONE,
	C2_WRITE_SFR,
	C2_WRITE_DIRECT, /* for devices with SFR paging */
	C2_READ_SFR,
	C2_READ_DIRECT, /* for devices with SFR paging */
	C2_WAIT_US,
};

static struct c2_setupcmd c2init_t63x[] = {
	{ C2_WRITE_DIRECT, 0xcf, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_WRITE_DIRECT, 0xb2, 0x83 }, /* set intosc (oscillator initialization) to 24.5 MHz */
	{ C2_DONE },
};

struct c2family families [] = {
	// DEVICE ID, Family name?, memory type, page_size, ??, ??, FTDAP address, ???
	{ 0x17, "C8051T63x", C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_t63x },
};

static int process_setupcmd(struct c2tool_state *state,
			    struct c2_setupcmd *setupcmd)
{
	struct c2interface *c2if = &state->c2if;
	int res;

	switch (setupcmd->token) {
	case C2_DONE:
		return 1;
	case C2_WRITE_SFR:
		res = c2_write_sfr(c2if, setupcmd->reg, setupcmd->value);
		break;
	case C2_WRITE_DIRECT:
		res = c2_write_direct(state, setupcmd->reg, setupcmd->value);
		break;
	case C2_READ_SFR:
		res = c2_read_sfr(c2if, setupcmd->reg);
		break;
	case C2_READ_DIRECT:
		res = c2_read_direct(state, setupcmd->reg);
		break;
	case C2_WAIT_US:
		usleep(setupcmd->time);
		res = 0;
		break;
	default:
		res = -1;
		break;
	}

	return res;
}

int c2family_find(unsigned int device_id, struct c2family **family)
{
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(families); ++k) {
		if (families[k].device_id == device_id) {
			*family = &families[k];
			return 0;
		}
	}

	return -EINVAL;
}

int c2family_setup(struct c2tool_state *state)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;
	int res;
	struct c2_setupcmd *setupcmd = family->setup;

	if (!setupcmd)
		return 0;

	while ((res = process_setupcmd(state, setupcmd++)) == 0) {}

	if (res == 1)
		return 0;
	else
		return -EIO;
}
