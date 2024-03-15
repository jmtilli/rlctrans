#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libsimul.h"

//const double dt = 1e-7; // 100 ns
const double dt = 2e-8; // 20 ns

void set_switches(struct libsimul_ctx *ctx, int phase, int onoff)
{
	const char *S1, *S2;
	if (phase == 1)
	{
		S1 = "S1";
		S2 = "S2";
	}
	else if (phase == 2)
	{
		S1 = "S3";
		S2 = "S4";
	}
	else if (phase == 3)
	{
		S1 = "S5";
		S2 = "S6";
	}
	else
	{
		abort();
	}
	if (onoff)
	{
		set_switch_state(ctx, S1, 1);
		set_switch_state(ctx, S2, 0);
	}
	else
	{
		set_switch_state(ctx, S1, 0);
		set_switch_state(ctx, S2, 1);
	}
	recalc(ctx);
}

int main(int argc, char **argv)
{
	size_t i;
	int cnt_remain1 = 500;
	int cnt_remain2 = 500;
	int cnt_remain3 = 500;
	const double f = 50;
	const double switch_f = 100e3;
	double curduty;
	const double V_in = 400;
	const double V_sine = 325.27;
	const double pi = 3.14159265358979;
	double t = 0;
	double ontime1 = 1e-5, offtime1 = 1e-5, V_out_ideal;
	double ontime2 = 1e-5, offtime2 = 1e-5;
	double ontime3 = 1e-5, offtime3 = 1e-5;
	int onoff1 = 0, onoff2 = 0, onoff3 = 0;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "inverterpwm3.txt");
	init_simulation(&ctx);
	set_switches(&ctx, 1, 1);
	set_switches(&ctx, 2, 1);
	set_switches(&ctx, 3, 1);
	cnt_remain1 = 1;
	cnt_remain2 = 1;
	cnt_remain3 = 1;
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		t += dt;
		printf("%zu %g %g %g\n", i, get_V(&ctx, 8)-get_V(&ctx, 9),
				get_V(&ctx, 9)-get_V(&ctx, 10),
				get_V(&ctx, 10)-get_V(&ctx, 8));
		cnt_remain1--;
		cnt_remain2--;
		cnt_remain3--;
		if (cnt_remain1 == 0 && onoff1 == 1)
		{
			set_switches(&ctx, 1, 0);
			cnt_remain1 = offtime1/dt;
			onoff1 = 0;
		}
		else if (cnt_remain1 == 0 && onoff1 == 0)
		{
			double ph = 0;
			V_out_ideal = V_sine*sin(2*pi*f*t + ph);
			V_out_ideal += V_sine*sin(2*pi*f*(t+1/70e3) + ph);
			V_out_ideal /= 2;
			curduty = 1.0 - (V_in - V_out_ideal)/(2*V_in);
			ontime1 = curduty/switch_f;
			offtime1 = (1.0-curduty)/switch_f;
			cnt_remain1 = ontime1/dt;
			set_switches(&ctx, 1, 1);
			onoff1 = 1;
		}
		if (cnt_remain2 == 0 && onoff2 == 1)
		{
			set_switches(&ctx, 2, 0);
			cnt_remain2 = offtime2/dt;
			onoff2 = 0;
		}
		else if (cnt_remain2 == 0 && onoff2 == 0)
		{
			double ph = 2*pi/3;
			V_out_ideal = V_sine*sin(2*pi*f*t + ph);
			V_out_ideal += V_sine*sin(2*pi*f*(t+1/70e3) + ph);
			V_out_ideal /= 2;
			curduty = 1.0 - (V_in - V_out_ideal)/(2*V_in);
			ontime2 = curduty/switch_f;
			offtime2 = (1.0-curduty)/switch_f;
			cnt_remain2 = ontime2/dt;
			set_switches(&ctx, 2, 1);
			onoff2 = 1;
		}
		if (cnt_remain3 == 0 && onoff3 == 1)
		{
			set_switches(&ctx, 3, 0);
			cnt_remain3 = offtime3/dt;
			onoff3 = 0;
		}
		else if (cnt_remain3 == 0 && onoff3 == 0)
		{
			double ph = 4*pi/3;
			V_out_ideal = V_sine*sin(2*pi*f*t + ph);
			V_out_ideal += V_sine*sin(2*pi*f*(t+1/70e3) + ph);
			V_out_ideal /= 2;
			curduty = 1.0 - (V_in - V_out_ideal)/(2*V_in);
			ontime3 = curduty/switch_f;
			offtime3 = (1.0-curduty)/switch_f;
			cnt_remain3 = ontime3/dt;
			set_switches(&ctx, 3, 1);
			onoff3 = 1;
		}
	}
	libsimul_free(&ctx);
	return 0;
}
