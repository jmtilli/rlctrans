#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns
const double diode_threshold = 1e-6;

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	read_file("rectifier.txt");
	init_simulation();
	for (i = 0; i < 5*1000*1000; i++)
	{
		set_voltage_source("V1", 24.0*sin(2*3.14159265358979*50.0*t));
		if (set_resistor("RL", 10.0*(1+0.3*sin(2*3.14159265358979*75.0*t))) != 0)
		{
			recalc();
		}
		t += dt;
		simulation_step();
		printf("%zu %g\n", i, get_V(2)-get_V(3));
	}
	return 0;
}
