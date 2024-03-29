#include <stdio.h>
#include <stdlib.h>
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
	const double V_tgt = 230*sqrt(2)*sqrt(3)*1.2;
	const double f_tgt = 50.0;
	double Vrms12 = 230*sqrt(3);
	double Vrms23 = 230*sqrt(3);
	double Vrms31 = 230*sqrt(3);
	double Vrms_accumulator12 = 0;
	int Vrms_cnt12 = 0;
	double Vrms_accumulator23 = 0;
	int Vrms_cnt23 = 0;
	double Vrms_accumulator31 = 0;
	int cnt_remain1 = 90, cnt_remain2 = 90, cnt_remain3 = 90;
	int cnt_on1 = 0, cnt_on2 = 0, cnt_on3 = 0;
	int Vrms_cnt31 = 0;
	int sw1 = 1, sw2 = 1, sw3 = 1;
	int sign_last_nonzero12 = 0, sign_last_nonzero23 = 0;
	int sign_last_nonzero31 = 0;
	int cycle_cnt12 = 0, cycle_cnt23 = 0, cycle_cnt31 = 0;
	double C;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "pfc3.txt");
	init_simulation(&ctx);
	set_switch_state(&ctx, "S1", sw1);
	set_switch_state(&ctx, "S2", sw2);
	set_switch_state(&ctx, "S3", sw3);
	C = get_capacitor(&ctx, "C1");
	recalc(&ctx);
	for (i = 0; i < 5*1000*1000; i++)
	{
		double IV1, IV2, IV3;
		double phi = 2*3.14159265358979*50.0*t;
		double V1, V2, V3;
		V1 = sqrt(2.0)*230.0*sin(phi);
		V2 = sqrt(2.0)*230.0*sin(phi+2*3.14159265358979/3);
		V3 = sqrt(2.0)*230.0*sin(phi+4*3.14159265358979/3);
		Vrms_accumulator12 += (V1-V2)*(V1-V2);
		Vrms_cnt12++;
		Vrms_accumulator23 += (V2-V3)*(V2-V3);
		Vrms_cnt23++;
		Vrms_accumulator31 += (V3-V1)*(V3-V1);
		Vrms_cnt31++;
		double R = get_resistor(&ctx, "RL");
		double V_out = get_V(&ctx, 10) - get_V(&ctx, 11);
		double I_R = V_out/R;
		double I_diff = C*(V_tgt-V_out)*2*f_tgt;
		double I_ideal12 = (I_R + I_diff)*fabs(V1-V2)/Vrms12;
		double I_ideal23 = (I_R + I_diff)*fabs(V2-V3)/Vrms23;
		double I_ideal31 = (I_R + I_diff)*fabs(V3-V1)/Vrms31;
		// x = 0..pi/3: V1 V2 positive V3 negative
		// x = pi/3..2*pi/3: V1 positive V2 V3 negative
		// x = 2*pi/3 .. 3*pi/3: V1 V3 positive V2 negative
		// x = 3*pi/3 .. 4*pi/3: V3 positive V1 V2 negative
		// x = 4*pi/3 .. 5*pi/3: V2 V3 positive V1 negative
		// x = 5*pi/3 .. 6*pi/3: V2 positive V1 V3 negative
		set_voltage_source(&ctx, "V1", V1);
		set_voltage_source(&ctx, "V2", V2);
		set_voltage_source(&ctx, "V3", V3);
		t += dt;
		simulation_step(&ctx);
		IV1 = get_voltage_source_current(&ctx, "V1");
		IV2 = get_voltage_source_current(&ctx, "V2");
		IV3 = get_voltage_source_current(&ctx, "V3");
		double IL1 = (get_inductor_current(&ctx, "L1b")-get_inductor_current(&ctx, "L1a"))/2;
		double IL2 = (get_inductor_current(&ctx, "L2b")-get_inductor_current(&ctx, "L2a"))/2;
		double IL3 = (get_inductor_current(&ctx, "L3b")-get_inductor_current(&ctx, "L3a"))/2;
		V_out = get_V(&ctx, 10) - get_V(&ctx, 11);
		printf("%zu %g %g %g %g %g %g %g\n", i, V_out, IV1, IL1, IV2, IL2, IV3, IL3);
		if (my_signum(V1-V2) != 0)
		{
			if (sign_last_nonzero12 != my_signum(V1-V2))
			{
				cycle_cnt12++;
				if (Vrms_cnt12 > 0.5/(2*f_tgt)/dt && cycle_cnt12 >= 2)
				{
					Vrms12 = sqrt(Vrms_accumulator12/Vrms_cnt12);
					Vrms_accumulator12 = 0;
					Vrms_cnt12 = 0;
					if (Vrms12 < 230*sqrt(3)*0.9)
					{
						Vrms12 = 230*sqrt(3)*0.9;
					}
					else if (Vrms12 > 230*sqrt(3)*1.1)
					{
						Vrms12 = 230*sqrt(3)*1.1;
					}
				}
				if (0 && rand() % 5 == 0)
				{
					if (set_resistor(&ctx, "RL", 500 + rand()%1000) != 0)
					{
						recalc(&ctx);
					}
				}
			}
			sign_last_nonzero12 = my_signum(V1-V2);
		}
		if (my_signum(V2-V3) != 0)
		{
			if (sign_last_nonzero23 != my_signum(V2-V3))
			{
				cycle_cnt23++;
				if (Vrms_cnt23 > 0.5/(2*f_tgt)/dt && cycle_cnt23 >= 2)
				{
					Vrms23 = sqrt(Vrms_accumulator23/Vrms_cnt23);
					Vrms_accumulator23 = 0;
					Vrms_cnt23 = 0;
					if (Vrms23 < 230*sqrt(3)*0.9)
					{
						Vrms23 = 230*sqrt(3)*0.9;
					}
					else if (Vrms23 > 230*sqrt(3)*1.1)
					{
						Vrms23 = 230*sqrt(3)*1.1;
					}
				}
				if (0 && rand() % 5 == 0)
				{
					if (set_resistor(&ctx, "RL", 500 + rand()%1000) != 0)
					{
						recalc(&ctx);
					}
				}
			}
			sign_last_nonzero23 = my_signum(V2-V3);
		}
		if (my_signum(V3-V1) != 0)
		{
			if (sign_last_nonzero31 != my_signum(V3-V1))
			{
				cycle_cnt31++;
				if (Vrms_cnt31 > 0.5/(2*f_tgt)/dt && cycle_cnt31 >= 2)
				{
					Vrms31 = sqrt(Vrms_accumulator31/Vrms_cnt31);
					Vrms_accumulator31 = 0;
					Vrms_cnt31 = 0;
					if (Vrms31 < 230*sqrt(3)*0.9)
					{
						Vrms31 = 230*sqrt(3)*0.9;
					}
					else if (Vrms31 > 230*sqrt(3)*1.1)
					{
						Vrms31 = 230*sqrt(3)*1.1;
					}
				}
				if (0 && rand() % 5 == 0)
				{
					if (set_resistor(&ctx, "RL", 500 + rand()%1000) != 0)
					{
						recalc(&ctx);
					}
				}
			}
			sign_last_nonzero31 = my_signum(V3-V1);
		}

		cnt_remain1--;
		cnt_remain2--;
		cnt_remain3--;

		if (sw1 && IL1 > I_ideal12 + 0.01)
		{
			cnt_remain1 = 0;
		}
		else if (!sw1 && IL1 < I_ideal12 - 0.01)
		{
			cnt_remain1 = 0;
		}
		if (sw1)
		{
			cnt_on1++;
		}
		if (cnt_remain1 == 0)
		{
			sw1 = !sw1;
			if (set_switch_state(&ctx, "S1", sw1) != 0)
			{
				recalc(&ctx);
			}
			if (sw1)
			{
				cnt_remain1 = 100000;
				cnt_on1 = 0;
			}
			else
			{
				cnt_remain1 = 100000;
			}
		}

		if (sw2 && IL2 > I_ideal23 + 0.01)
		{
			cnt_remain2 = 0;
		}
		else if (!sw2 && IL2 < I_ideal23 - 0.01)
		{
			cnt_remain2 = 0;
		}
		if (sw2)
		{
			cnt_on2++;
		}
		if (cnt_remain2 == 0)
		{
			sw2 = !sw2;
			if (set_switch_state(&ctx, "S2", sw2) != 0)
			{
				recalc(&ctx);
			}
			if (sw2)
			{
				cnt_remain2 = 100000;
				cnt_on2 = 0;
			}
			else
			{
				cnt_remain2 = 100000;
			}
		}

		if (sw3 && IL3 > I_ideal31 + 0.01)
		{
			cnt_remain3 = 0;
		}
		else if (!sw3 && IL3 < I_ideal31 - 0.01)
		{
			cnt_remain3 = 0;
		}
		if (sw3)
		{
			cnt_on3++;
		}
		if (cnt_remain3 == 0)
		{
			sw3 = !sw3;
			if (set_switch_state(&ctx, "S3", sw3) != 0)
			{
				recalc(&ctx);
			}
			if (sw3)
			{
				cnt_remain3 = 100000;
				cnt_on3 = 0;
			}
			else
			{
				cnt_remain3 = 100000;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
