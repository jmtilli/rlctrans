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
	double last_I_xfr = 0;
	const double V_tgt = 60;
	const char *xformer = "X1";
	libsimul_init(&ctx, dt);
	read_file(&ctx, "flybackgood.txt");
	init_simulation(&ctx);
	L = get_transformer_inductor(&ctx, xformer);
	C = get_capacitor(&ctx, "C1");
	R = get_resistor(&ctx, "RL");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		set_diode_hint(&ctx, "D1", !switch_state);
		recalc(&ctx);
	}
	for (i = 0; i < 3*1000*1000; i++)
	{
		simulation_step(&ctx);
		double I_xfr = get_transformer_mag_current(&ctx, xformer);
		double V_out = get_V(&ctx, 5) - get_V(&ctx, 3);
		double E_switch = V_tgt*V_tgt/R*dt*1000;
		double V_new = sqrt(V_out*V_out + L/C*I_xfr*I_xfr - L/C*last_I_xfr*last_I_xfr - 2*E_switch/C);
		//double V_new = sqrt(V_out*V_out + L/C*I_xfr*I_xfr - 2*E_switch/C);
		printf("%zu %g %g\n", i, V_out, I_xfr);
		cnt_remain--;
		if (switch_state && (V_new > V_tgt || I_xfr > 15))
		{
			cnt_on++;
			cnt_remain = 0;
			//printf("%zu cnt_on %d\n", i, cnt_on);
		}
		else if (switch_state)
		{
			cnt_on++;
		}
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			//printf("Switch state to %d\n", switch_state);
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				set_diode_hint(&ctx, "D1", !switch_state);
				recalc(&ctx);
			}
			if (switch_state)
			{
				last_I_xfr = 0.99*last_I_xfr+0.01*I_xfr;
				//fprintf(stderr, "last_I_xfr %g\n", last_I_xfr);
				cnt_remain = 950;
				cnt_on = 0;
			}
			else
			{
				cnt_remain = 1000-cnt_on;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
