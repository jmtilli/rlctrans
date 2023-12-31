#include <stdio.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "flyback.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		set_diode_hint(&ctx, "D1", !switch_state);
		recalc(&ctx);
	}
	for (i = 0; i < 3*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 5) - get_V(&ctx, 3));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			//switch_state = 1;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				set_diode_hint(&ctx, "D1", !switch_state);
				recalc(&ctx);
			}
			if (switch_state)
			{
				cnt_remain = 500;
			}
			else
			{
				cnt_remain = 100;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
