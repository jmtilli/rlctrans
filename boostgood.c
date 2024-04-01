#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 2e-8; // 20 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	int cnt_on = 0;
	struct libsimul_ctx ctx;
	double C, L, R;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "boost.txt");
	init_simulation(&ctx);
	L = get_inductor(&ctx, "L1");
	C = get_capacitor(&ctx, "C1");
	R = get_resistor(&ctx, "RL");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		double I_ind = -get_inductor_current(&ctx, "L1");
		printf("%zu %g\n", i, get_V(&ctx, 3));
		//double I_RL = (get_V(&ctx, 3) - get_V(&ctx, 0))/R;
		double I_RL = 6.6*2/R;
		double V_out = get_V(&ctx, 3);
		double V_new = sqrt(V_out*V_out + L/C*(I_ind-I_RL)*(I_ind-I_RL));
		cnt_remain--;
		if (switch_state && (V_new > 6.6*2*1.004 || I_ind > 3))
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
