#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "rectifier3.txt");
	init_simulation(&ctx);
	for (i = 0; i < 5*1000*1000; i++)
	{
		set_voltage_source(&ctx, "V1", sqrt(2.0)*230.0*sin(2*3.14159265358979*50.0*t));
		set_voltage_source(&ctx, "V2", sqrt(2.0)*230.0*sin(2*3.14159265358979*50.0*t+2*3.14159265358979/3));
		set_voltage_source(&ctx, "V3", sqrt(2.0)*230.0*sin(2*3.14159265358979*50.0*t+4*3.14159265358979/3));
		if (set_resistor(&ctx, "RL", 1000.0*(1+0.3*sin(2*3.14159265358979*75.0*t))) != 0)
		{
			recalc(&ctx);
		}
		t += dt;
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 4)-get_V(&ctx, 5));
	}
	libsimul_free(&ctx);
	return 0;
}
