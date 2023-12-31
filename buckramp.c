#include <stdio.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns
double duty_cycle = 0.02;

void recalc_dc(void)
{
	double new_dc;
	new_dc = duty_cycle+0.0001;
	if (new_dc > 0.5)
	{
		new_dc = 0.5;
	}
	duty_cycle = new_dc;
}

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 100*duty_cycle;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "buckramp.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc_dc();
		cnt_remain = 100*duty_cycle;
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 4));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			//switch_state = 1;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				recalc(&ctx);
			}
			if (switch_state)
			{
				recalc_dc();
				cnt_remain = 100*duty_cycle;
			}
			else
			{
				cnt_remain = 100*(1-duty_cycle);
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
