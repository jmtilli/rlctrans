#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	int cnt_on = 0;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "buckgood.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 4));
		//double I_ind = (get_V(&ctx, 3)-get_V(&ctx, 4))/100e-3;
		double I_ind = get_inductor_current(&ctx, "L1");
		double V_out = get_V(&ctx, 4);
		const double C = 2200e-6;
		const double L = 300e-6;
		double V_new = sqrt(V_out*V_out + L/C*I_ind*I_ind);
		cnt_remain--;
		if (switch_state && V_new > 13.2*0.5*1.004)
		{
			cnt_on++;
			cnt_remain = 0;
		}
		else if (switch_state)
		{
			cnt_on++;
		}
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
				cnt_remain = 950;
				cnt_on = 0;
			}
			else
			{
				// Constant frequency
				cnt_remain = 1000 - cnt_on;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
