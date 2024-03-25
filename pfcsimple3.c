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

static void set_phase_switches(struct libsimul_ctx *ctx, int phase, int onoff)
{
	if (phase == 1)
	{
		set_switch_state(ctx, "S1", !!onoff);
		set_switch_state(ctx, "S2", !onoff);
	}
	else if (phase == 2)
	{
		set_switch_state(ctx, "S3", !!onoff);
		set_switch_state(ctx, "S4", !onoff);
	}
	else if (phase == 3)
	{
		set_switch_state(ctx, "S5", !!onoff);
		set_switch_state(ctx, "S6", !onoff);
	}
	recalc(ctx);
}

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	const double pi = 3.14159265358979;
	const double f_tgt = 50.0;
	double Vrms1 = 230;
	double Vrms2 = 230;
	double Vrms3 = 230;
	double Vrms_accumulator1 = 0;
	double Vrms_accumulator2 = 0;
	double Vrms_accumulator3 = 0;
	int Vrms_cnt1 = 0;
	int Vrms_cnt2 = 0;
	int Vrms_cnt3 = 0;
	int sign_last_nonzero1 = 0;
	int sign_last_nonzero2 = 0;
	int sign_last_nonzero3 = 0;
	int cycle_cnt1 = 0;
	int cycle_cnt2 = 0;
	int cycle_cnt3 = 0;
	const double V_tgt = 230*sqrt(2)*sqrt(3)*1.2;
	double C;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "pfcsimple3.txt");
	init_simulation(&ctx);
	C = get_capacitor(&ctx, "C1");
	set_phase_switches(&ctx, 1, 1); // either 1 or 0 can work
	set_phase_switches(&ctx, 2, 0);
	set_phase_switches(&ctx, 3, 1);
	for (i = 0; i < 5*1000*1000; i++)
	{
		double V1, V2, V3;
		V1 = 230.0*sqrt(2.0)*sin(2*pi*50*t+0.0);
		V2 = 230.0*sqrt(2.0)*sin(2*pi*50*t+2*pi/3);
		V3 = 230.0*sqrt(2.0)*sin(2*pi*50*t+4*pi/3);
		Vrms_accumulator1 += V1*V1;
		Vrms_cnt1++;
		Vrms_accumulator2 += V2*V2;
		Vrms_cnt2++;
		Vrms_accumulator3 += V3*V3;
		Vrms_cnt3++;
		double R = get_resistor(&ctx, "RL");
		double V_out = get_V(&ctx, 10) - get_V(&ctx, 11);
		double I_R = V_out/R * 1.2 * sqrt(2.0);
		double I_diff = C*(V_tgt-V_out)*2*f_tgt;
		double I_ideal1 = (I_R + I_diff)*V1/Vrms1;
		double I_ideal2 = (I_R + I_diff)*V2/Vrms2;
		double I_ideal3 = (I_R + I_diff)*V3/Vrms3;
		set_voltage_source(&ctx, "V1", V1);
		set_voltage_source(&ctx, "V2", V2);
		set_voltage_source(&ctx, "V3", V3);
		t += dt;
		simulation_step(&ctx);
		double IL1 = -get_inductor_current(&ctx, "L1");
		double IL2 = -get_inductor_current(&ctx, "L2");
		double IL3 = -get_inductor_current(&ctx, "L3");
		V_out = get_V(&ctx, 10) - get_V(&ctx, 11);
		printf("%zu %g %g %g %g %g %g %g\n", i, V_out, IL1, I_ideal1, IL2, I_ideal2, IL3, I_ideal3);
		if (my_signum(V1) != 0)
		{
			if (sign_last_nonzero1 != my_signum(V1))
			{
				cycle_cnt1++;
				if (Vrms_cnt1 > 0.5/(2*f_tgt)/dt && cycle_cnt1 >= 2)
				{
					Vrms1 = sqrt(Vrms_accumulator1/Vrms_cnt1);
					Vrms_accumulator1 = 0;
					Vrms1 = 0;
					if (Vrms1 < 230*0.9)
					{
						Vrms1 = 230*0.9;
					}
					else if (Vrms1 > 230*1.1)
					{
						Vrms1 = 230*1.1;
					}
				}
			}
			sign_last_nonzero1 = my_signum(V1);
		}
		if (my_signum(V2) != 0)
		{
			if (sign_last_nonzero2 != my_signum(V2))
			{
				cycle_cnt2++;
				if (Vrms_cnt2 > 0.5/(2*f_tgt)/dt && cycle_cnt2 >= 2)
				{
					Vrms2 = sqrt(Vrms_accumulator2/Vrms_cnt2);
					Vrms_accumulator2 = 0;
					Vrms2 = 0;
					if (Vrms2 < 230*0.9)
					{
						Vrms2 = 230*0.9;
					}
					else if (Vrms2 > 230*1.1)
					{
						Vrms2 = 230*1.1;
					}
				}
			}
			sign_last_nonzero2 = my_signum(V2);
		}
		if (my_signum(V3) != 0)
		{
			if (sign_last_nonzero3 != my_signum(V3))
			{
				cycle_cnt3++;
				if (Vrms_cnt3 > 0.5/(2*f_tgt)/dt && cycle_cnt3 >= 2)
				{
					Vrms3 = sqrt(Vrms_accumulator3/Vrms_cnt3);
					Vrms_accumulator3 = 0;
					Vrms3 = 0;
					if (Vrms3 < 230*0.9)
					{
						Vrms3 = 230*0.9;
					}
					else if (Vrms3 > 230*1.1)
					{
						Vrms3 = 230*1.1;
					}
				}
			}
			sign_last_nonzero3 = my_signum(V3);
		}
		double Idiff1, Idiff2, Idiff3;
		double adiff1, adiff2, adiff3;
		int onoff1 = -1, onoff2 = -1, onoff3 = -1;
		Idiff1 = IL1 - I_ideal1;
		Idiff2 = IL2 - I_ideal2;
		Idiff3 = IL3 - I_ideal3;
		adiff1 = fabs(Idiff1);
		adiff2 = fabs(Idiff2);
		adiff3 = fabs(Idiff3);
		if (adiff1 > 0.001 || adiff2 > 0.001 || adiff3 > 0.001)
		{
			onoff1 = (Idiff1 > 0);
			onoff2 = (Idiff2 > 0);
			onoff3 = (Idiff3 > 0);
			if (onoff1 && onoff2 && onoff3 && 0)
			{
				if (Idiff1 < Idiff2 && Idiff1 < Idiff3)
				{
					onoff1 = 0;
				}
				else if (Idiff2 < Idiff3 && Idiff2 < Idiff1)
				{
					onoff2 = 0;
				}
				else
				{
					onoff3 = 0;
				}
				//fprintf(stderr, "All on\n");
				//abort();
			}
			if (!onoff1 && !onoff2 && !onoff3 && 0)
			{
				if (Idiff1 > Idiff2 && Idiff1 > Idiff3)
				{
					onoff1 = 1;
				}
				else if (Idiff2 > Idiff3 && Idiff2 > Idiff1)
				{
					onoff2 = 1;
				}
				else
				{
					onoff3 = 1;
				}
				//fprintf(stderr, "All off\n");
				//abort();
			}
			set_phase_switches(&ctx, 1, onoff1);
			set_phase_switches(&ctx, 2, onoff2);
			set_phase_switches(&ctx, 3, onoff3);
		}
	}
	libsimul_free(&ctx);
	return 0;
}
