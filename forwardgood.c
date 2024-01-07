#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 2e-8; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	int cnt_on = 0;
	double L, C;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "forwardgood.txt");
	init_simulation(&ctx);
	L = get_inductor(&ctx, "L1");
	C = get_capacitor(&ctx, "C1");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 3*1000*1000; i++)
	{
		double V_out, I_ind, V_new;
		simulation_step(&ctx);
		V_out = get_V(&ctx, 7) - get_V(&ctx, 4);
		I_ind = get_inductor_current(&ctx, "L1");
		V_new = sqrt(V_out*V_out + L/C*I_ind*I_ind);
		printf("%zu %g\n", i, get_V(&ctx, 7) - get_V(&ctx, 4));
		cnt_remain--;
		if (switch_state && V_new > 24*0.5*0.5*1.001)
		{
			cnt_on++;
			//printf("i %zu cnt_on %d\n", i, cnt_on);
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
				cnt_remain = 530;
				cnt_on = 0;
			}
			else
			{
				cnt_remain = 1000 - cnt_on;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
