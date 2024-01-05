#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

static inline int my_signum(double d)
{
	if (d > 0)
	{
		return 1;
	}
	if (d < 0)
	{
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	const double pi = 3.14159265358979;
	int switch_state = 1;
	int cnt_remain = 90;
	int cnt_on = 0;
	double I_ideal_rms_230 = 0.5;
	double I_diff_single = 0.0;
	//double V_input_last = 0;
	int sign_last_nonzero = 0;
	//double last_V = 0;
	const double V_tgt = 230*sqrt(2)*1.2;
	double C;//, L;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "pfcboost.txt");
	init_simulation(&ctx);
	//L = get_inductor(&ctx, "L2");
	C = get_capacitor(&ctx, "C2");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		double V_input = 230*sqrt(2)*sin(2*pi*50*t);
		double I_ideal = (I_ideal_rms_230+I_diff_single)*fabs(V_input)/230;
		set_voltage_source(&ctx, "V1", V_input);
		t += dt;
		simulation_step(&ctx);
		//double I_input = get_inductor_current(&ctx, "L1");
		double I_ind = -get_inductor_current(&ctx, "L2");
		double V_rect = get_V(&ctx, 2) - get_V(&ctx, 3);
		double V_out = get_V(&ctx, 6) - get_V(&ctx, 3);
		printf("%zu %g (%d) %g %g %g\n", i, V_input, switch_state, V_out, V_rect, I_ind);
		//double V_new = sqrt(V_out*V_out + L/C*I_ind*I_ind);
		if (my_signum(V_input) != 0)
		{
			if (sign_last_nonzero != my_signum(V_input))
			{
				double E_cap;
				double E_ideal;
				//last_V = V_out;
				E_cap = 0.5*C*V_out*V_out;
				E_ideal = 0.5*C*V_tgt*V_tgt;
				I_diff_single = (E_ideal-E_cap)*2*50.0/230.0;
				I_ideal_rms_230 += I_diff_single/20.0;
				I_diff_single = 19.0/20.0*I_diff_single;
			}
			sign_last_nonzero = my_signum(V_input);
		}
		//V_input_last = V_input;
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
