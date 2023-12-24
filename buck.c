#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lapack.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	read_file("buck.txt");
	init_simulation();
	if (set_switch_state("S1", switch_state) != 0)
	{
		recalc();
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step();
		printf("%zu %g\n", i, get_V(4));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			//switch_state = 1;
			if (set_switch_state("S1", switch_state) != 0)
			{
				recalc();
			}
			if (switch_state)
			{
				cnt_remain = 500;
			}
			else
			{
				cnt_remain = 500;
			}
		}
	}
	return 0;
}
