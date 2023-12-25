#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns
const double diode_threshold = 0;

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt, diode_threshold);
	read_file(&ctx, "transformer.txt");
	init_simulation(&ctx);
	for (i = 0; i < 5*1000*1000; i++)
	{
		set_voltage_source(&ctx, "V1", 24.0*sin(2*3.14159265358979*50.0*t));
		t += dt;
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 3) - get_V(&ctx, 2));
	}
	return 0;
}
