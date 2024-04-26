#include <stdio.h>
#include <errno.h>

#include "c2family.h"
#include "c2tool.h"

int handle_reset(struct c2tool_state *state, int argc, char **argv)
{
	struct c2interface *c2if = &state->c2if;

	c2_reset(c2if);

	return 0;
}

COMMAND(reset, NULL, handle_reset, "Reset connected device.");
