#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns
const double diode_threshold = 0;

void set_switches(int onoff)
{
	if (onoff)
	{
		set_switch_state("S1", 1);
		set_switch_state("S2", 1);
		set_switch_state("S3", 0);
		set_switch_state("S4", 0);
	}
	else
	{
		set_switch_state("S1", 0);
		set_switch_state("S2", 0);
		set_switch_state("S3", 1);
		set_switch_state("S4", 1);
	}
	recalc();
}

int main(int argc, char **argv)
{
	size_t i;
	int cnt_remain = 500;
	const double f = 50;
	const double switch_f = 100e3;
	double curduty;
	const double V_in = 400;
	const double V_sine = 325.27;
	const double pi = 3.14159265358979;
	double t = 0;
	double ontime = 1e-5, offtime = 1e-5, V_out_ideal;
	int onoff = 0;
	read_file("inverterpwm.txt");
	init_simulation();
	set_switches(1);
	cnt_remain = 1;
	for (i = 0; i < 1000*1000; i++)
	{
		simulation_step();
		t += dt;
		printf("%zu %g\n", i, get_V(5)-get_V(3));
		cnt_remain--;
		if (cnt_remain == 0 && onoff == 1)
		{
			set_switches(0);
			cnt_remain = offtime/dt;
			onoff = 0;
		}
		if (cnt_remain == 0 && onoff == 0)
		{
                        V_out_ideal = V_sine*sin(2*pi*f*t);
                        V_out_ideal += V_sine*sin(2*pi*f*(t+1/70e3));
			V_out_ideal /= 2;
			curduty = 1.0 - (V_in - V_out_ideal)/(2*V_in);
			ontime = curduty/switch_f;
			offtime = (1.0-curduty)/switch_f;
			cnt_remain = ontime/dt;
			set_switches(1);
			onoff = 1;
		}
	}
	return 0;
}
