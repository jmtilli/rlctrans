#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	const double pi = 3.14159265358979;
	int switch_state = 1;
	int cnt_remain = 90;
	int cnt_on = 0;
	//double C, L;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "pfcboost.txt");
	init_simulation(&ctx);
	//L = get_inductor(&ctx, "L2");
	//C = get_capacitor(&ctx, "C2");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		double V_input = 230*sqrt(2)*sin(2*pi*50*t);
		double I_ideal = 0.5*fabs(V_input)/230;
		set_voltage_source(&ctx, "V1", V_input);
		t += dt;
		simulation_step(&ctx);
		//double I_input = get_inductor_current(&ctx, "L1");
		double I_ind = -get_inductor_current(&ctx, "L2");
		double V_rect = get_V(&ctx, 2) - get_V(&ctx, 3);
		double V_out = get_V(&ctx, 6) - get_V(&ctx, 3);
		printf("%zu %g (%d) %g %g %g\n", i, V_input, switch_state, V_out, V_rect, I_ind);
		//double V_new = sqrt(V_out*V_out + L/C*I_ind*I_ind);
		cnt_remain--;
		if (switch_state && I_ind > I_ideal + 0.01)
		{
			cnt_remain = 0;
		}
		else if (!switch_state && I_ind < I_ideal - 0.01)
		{
			cnt_remain = 0;
		}
#if 0
		if (switch_state && V_new > 400)
		{
			//printf("i %zu cnt_on %d\n", i, cnt_on);
			cnt_remain = 0;
		}
#endif
		if (switch_state)
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
				cnt_remain = 90;
				cnt_on = 0;
				//printf("Cnt_remain set to %d\n", cnt_remain);
			}
			else
			{
				// Constant frequency
				cnt_remain = 100 - cnt_on;
				//printf("cnt_remain set to %d\n", cnt_remain);
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
