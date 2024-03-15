# RLCTrans

RLCTrans is a transient analysis program for circuits where the most important
elements are resistors, inductors and capacitors, and where switches (either
programmatically controlled or automatic diodes) are present. Transformers are
handled by a linear matrix model (potentially multiple transformers per
circuit) or by binary search of the voltages that result in continuous core
magnetic flux (one transformer per circuit only permitted in this case). The
analysis method is nodal analysis (with capacitors, inductors, transformers and
voltage sources converted to current sources with possible shunt resistor). The
program is ideal for analyzing control algorithms and core saturation of
switched mode power supplies and active power factor correction circuits. The
only form of nonlinearities supported are either changing component values from
the C code as the simulation runs (e.g. inductor core saturation by decreasing
inductance from the C code, or modifying load resistance over time to simulate
varying power consumption), diodes and switches. Diode is somewhat idealized:
when it's off there's no leakage, and when it's on there's no voltage drop
apart from the drop caused by its resistance. Threshold voltage may be
simulated by putting a voltage source in series with the diode, but that's
still not the full Shockley diode equation.

MOSFETs may be simulated by a switch and possibly an antiparallel diode if
needed, and BJTs by a switch, a voltage source that provides constant voltage
drop and a diode that prevents reverse current. These models are therefore
rather crude.

The time step in transient analysis is a constant value that isn't changed
dynamically depending on the simulation conditions.

One aspect where RLCTrans totally fails is the determination of efficiency of
switched mode power supplies, because full MOSFET and BJT simulation is not
done, because transformer and inductor core losses are not modeled, and because
the momentary high resistance at switch-on and switch-off times of MOSFETs is
not modeled. Also further contributing to failure is the lack of nonlinear full
Shockley diode simulation.

Partial transformer support is present, but currently the count of transformers
in a circuit is limited to 1 if using the old transformer model. For new
transformer model, multiple transformers per circuit are permitted. However,
even with the old model, the single transformer may have an arbitrary number of
secondary windings, so for example forward converter with a reset winding may
be simulated.

The diode support may in some cases be somewhat unstable, since it may lead to
endless loops where diode switches are opened and closed alternately without
reaching a conclusion. Sometimes this may be solved by adding dummy resistors
with high values, but in the worst situation the solution may be replacement of
diodes with manually controlled switches that are controlled from C code.

Circuits where isolation is present are not supported. So for example you need
to provide a possibly high resistance connection between transformer primary
and secondary so that the voltages on the secondary side would not be floating.

A C code is needed for every simulation, although in most cases it requires
little customization. For switched mode power supplies, of course the full
control algorithm needs to be in the C code.

Because the program is based on nodal analysis, resistance cannot be omitted
for voltage sources, capacitors, switches, diodes and transformers. However,
the resistance can be a small dummy value such as 1 milliohm. Inductors may
not have a built-in resistance, but you may put a second resistor element in
series as the inductor winding resistance.

## Supported component types:

The component name should begin with any of these letters:

* `R` is a resistor (mandatory parameters: `R` for resistance)
* `L` is an inductor (mandatory parameters: `L` for inductance)
* `C` is a capacitor (mandatory parameters: `C` for capacitance, `R` for internal resistance)
* `V` is a voltage source (mandatory parameters: `V` for voltage, `R` for internal resistance)
* `D` is a diode (mandatory parameters: `R` for internal resistance, optional parameters: `diode_threshold` as the positive threshold voltage which avoids recalculation loops)
* `S` is a switch (mandatory parameters: `R` for internal resistance)
* `T` is a transformer winding using binary search model (mandatory parameters: `N` for turns ratio, `R` for internal resistance, `primary` for flag telling if it's primary winding (1) or secondary winding (0), and for primary windings too: `Lbase` for theoretical inductance if there was only one turn, `Vmin` for minimum search voltage, `Vmax` for maximum search voltage)
* `X` is a transformer winding using linear model (mandatory parameters: `N` for turns ratio, `R` for internal resistance, `primary` for flag telling if it's primary winding (1) or secondary winding (0), and for primary windings too: `Lbase` for theoretical inductance if there was only one turn)

## Some notes about failed simulations

If LU decomposition can't be done, most likely some nodes are isolated or
reachable only via an inductor current source. Adding a high-value resistor
between some nodes helps. This occurs e.g.

* For diode bridges: if all diodes are non-conducting, there is isolation,
  adding some high-value resistors somewhere may help but improperly chosen
  locations for the resistors can lead to recalculation loop
* For transformer circuits: transformers create isolation which needs to be
  broken by adding a high-value resistor from one side of primary to one side
  of secondary
* For circuits where some node is reachable only via an inductor and maybe
  a diode, but if the diode doesn't conduct, it's reachable only via current
  source i.e. the inductor: adding a high-value resistor across the inductor
  helps. Note that adding a high-value resistors across the diode doesn't help,
  since the high-value resistor R will create with the inductor L an RL circuit
  with time constant t=L/R which is extremely small since R is extremely large,
  and the simulation step is too long to simulate this circuit.

If recalculation loop happens, it generally means diodes don't reach a stable
state. It may happen in full wave bridge rectifiers. Adding a high-value
resistor across two diodes may help in this case, but adding only one resistor
or four resistors may cause the recalculation loop again. Also the
`diode_threshold` parameter for diodes which sets the threshold voltage after
which switch state reverses, may be useful. Note this is different than voltage
drop which is about 0.7 V for silicon P-N diodes. This threshold voltage should
be very small, generally on the order of microvolts.

In the worst case, if you can't avoid a recalculation loop, you need to replace
diodes with switches that you control from the C code.

## Note about transformers

Transformers with the old model (elements like `T1`) use a binary search and
you need to specify Vmin and Vmax for their primary. The binary search for the
primary voltage is made between these two values. Excessively large spacing
between Vmin and Vmax causes slowness; if the transformer would need to operate
at a point that is not between Vmin and Vmax, simulation fails.

Transformer primary has a parameter Lbase which is defined as: Lbase = L/N^2,
where L is the inductance of the primary winding and N is the count of wire
loops in the primary winding.

Transformers with the old model are currently limited to max 1 transformer per
circuit, but the transformer may have an unlimited number of secondary
windings. Secondary windings may not have Lbase, Vmin or Vmax; they are derived
automatically from the wire loop count N which must be present for all
transformer windings.

New model (elements like `X1`) does not have these restrictions, but results
sometimes in recalculation loop if multiple diodes in the circuit are present,
due to numerical inaccuracy. You may need to add a high-value bypass resistor
across some diode to prevent this recalculation loop. The new linear matrix
based transformer model is much faster.

## Note about OpenBLAS

Note that some versions of OpenBLAS require the environment variable:

```
export OPENBLAS_NUM_THREADS=1
```

...to work fast. Recent versions of OpenBLAS, however, automatically use only
one thread if the matrix is small, which it usually is.

Note that since transformers are handled by a binary search, the moment a
transformer is added to a circuit reduces performance markedly.

## How to build

RLCTrans is built using stirmake. How to build: first install byacc and flex.
Then install stirmake:

```
git clone https://github.com/Aalto5G/stirmake
cd stirmake
git submodule init
git submodule update
cd stirc
make
./install.sh
```

This installs stirmake to `~/.local`. If you want to install to `/usr/local`,
run `./install.sh` by typing `sudo ./install.sh /usr/local` (and you may want
to run `sudo mandb` also).

If the installation told `~/.local` is missing, create it with `mkdir` and try
again. If the installation needed to create `~/.local/bin`, you may need to
re-login for the programs to appear in your `PATH`.

Then ensure that some version of lapack along with its development headers is
installed and build RLCTrans by:

```
cd rlctrans
smka
```

## Examples

### Full-wave bridge rectifier

Circuit schematic:

![rectifier schematic](figs/rectifier.png)

Netlist rectifier.txt:

```
1 0 V1 V=0 R=1e-3
1 2 D1 R=1e-3
1 2 R1 R=1e10
0 2 D2 R=1e-3
3 1 D3 R=1e-3
3 1 R3 R=1e10
3 0 D4 R=1e-3
2 3 C1 C=2200e-6 R=1e-3 Vinit=0
2 3 RL R=10
```

Note the resistors R1 and R3 that have a large value and are in parallel with
diodes D1 and D3. The resistors with large value are needed to avoid isolation
if no diode is conducting, and to create asymmetry which prevents infinite
recalculation loops.

Main program which modifies value of `RL` dynamically:

```
#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	struct libsimul_ctx ctx;
	linsimul_init(&ctx, dt);
	read_file(&ctx, "rectifier.txt");
	init_simulation(&ctx);
	for (i = 0; i < 5*1000*1000; i++)
	{
		set_voltage_source(&ctx, "V1", 24.0*sin(2*3.14159265358979*50.0*t));
		if (set_resistor(&ctx, "RL", 10.0*(1+0.3*sin(2*3.14159265358979*75.0*t))) != 0)
		{
			recalc(&ctx);
		}
		t += dt;
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 2)-get_V(&ctx, 3));
	}
	return 0;
}
```

Plot of output:

![rectifier plot](plots/rectifierplot.png)

### Buck converter controlled by constant 50% duty cycle:

Circuit schematic:

![buck schematic](figs/buck.png)

Netlist buck.txt:

```
1 0 V1 V=13.2 R=1e-3
1 2 S1 R=1e-3
0 2 D1 R=1e-3
2 3 RRL1 R=1e9
2 3 L1 L=300e-6 Iinit=0
3 4 RL1 R=30.6e-3
4 0 C1 C=6600e-6 R=1e-3 Vinit=0
4 0 RL R=10
```

Note the resistor RRL1 with large value in parallel with L1. It is needed to
avoid isolating the end of L1 which is connected to the rest only via an
inductor that is essentially a constant current source whenever D1 is not
conducting and S1 is open.

Main program:

```
#include <stdio.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	struct libsimul_ctx ctx;
	linsimul_init(&ctx, dt);
	read_file(&ctx, "buck.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 4));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				recalc(&ctx);
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
```

Because of the constant duty cycle, there is significant overshoot in powering
up the buck converter. A more sophisticated control algorithm could avoid the
overshoot.

Plot of output:

![buck converter plot 50% duty cycle](plots/buckplot.png)

### Better ramp-up of buck converter

Circuit schematic:

![buck schematic](figs/buck.png)

If we modify the netlist to have more resistance and different component
values:

```
1 0 V1 V=13.2 R=1e-3
1 2 S1 R=1e-3
0 2 D1 R=1e-3
2 3 RRL1 R=1e9
2 3 L1 L=300e-6 Iinit=0
3 4 RL1 R=100e-3
4 0 C1 C=2200e-6 R=1e-3 Vinit=0
4 0 RL R=10
```

Note the resistor RRL1 with large value in parallel with L1. It is needed to
avoid isolating the end of L1 which is connected to the rest only via an
inductor that is essentially a constant current source whenever D1 is not
conducting and S1 is open.

...and have slow ramp-up in the control algorithm:

```
#include <stdio.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns
double duty_cycle = 0.02;

void recalc_dc(void)
{
	double new_dc;
	new_dc = duty_cycle+0.0001;
	if (new_dc > 0.5)
	{
		new_dc = 0.5;
	}
	duty_cycle = new_dc;
}

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 100*duty_cycle;
	struct libsimul_ctx ctx;
	linsimul_init(&ctx, dt);
	read_file(&ctx, "buckramp.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc_dc();
		cnt_remain = 100*duty_cycle;
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 4));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				recalc(&ctx);
			}
			if (switch_state)
			{
				recalc_dc();
				cnt_remain = 100*duty_cycle;
			}
			else
			{
				cnt_remain = 100*(1-duty_cycle);
			}
		}
	}
	return 0;
}
```

...it is possible to avoid the overshoot:

![buck converter plot ramp-up to 50% duty cycle](plots/buckrampplot.png)

### Physical modeling of buck converter

Circuit schematic:

![buck schematic](figs/buck.png)

A perfect control of buck converter is achieved by embedding a model of the
physical system into the control algorithm. Netlist `buckgood.txt`:

```
1 0 V1 V=13.2 R=1e-3
1 2 S1 R=1e-3
0 2 D1 R=1e-3
2 3 RRL1 R=1e9
2 3 L1 L=300e-6 Iinit=0
3 4 RL1 R=100e-3
4 0 C1 C=2200e-6 R=1e-3 Vinit=0
4 0 RL R=10
```

Source code:

```
#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	int cnt_on = 0;
	struct libsimul_ctx ctx;
	double C, L;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "buckgood.txt");
	init_simulation(&ctx);
	L = get_inductor(&ctx, "L1");
	C = get_capacitor(&ctx, "C1");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 5*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 4));
		double I_ind = get_inductor_current(&ctx, "L1");
		double V_out = get_V(&ctx, 4);
		double V_new = sqrt(V_out*V_out + L/C*I_ind*I_ind);
		cnt_remain--;
		if (switch_state && V_new > 13.2*0.5*1.004)
		{
			cnt_on++;
			cnt_remain = 0;
		}
		else if (switch_state)
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
				cnt_remain = 950;
				cnt_on = 0;
			}
			else
			{
				// Constant frequency
				cnt_remain = 1000 - cnt_on;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
```

Plot of perfectly controlled output:

![buck converter plot with good control](plots/buckgoodplot.png)

### AC transformer

Circuit schematic:

![transformer schematic](figs/transformer.png)

Netlist transformer.txt:

```
1 0 V1 V=0 R=1e-3
1 0 T1 N=100 primary=1 Lbase=1e-6 Vmin=-30 Vmax=30 R=60e-3
0 2 Rbypass R=1e10
3 2 T1 N=50 primary=0 R=60e-3
3 2 RL R=10
```

Alternative netlist for newer fast transformer model:

```
1 0 V1 V=0 R=1e-3
1 0 X1 N=100 primary=1 Lbase=1e-6 Vmin=-30 Vmax=30 R=60e-3
0 2 Rbypass R=1e10
3 2 X1 N=50 primary=0 R=60e-3
3 2 RL R=10
```

Note the bypass resistor Rbypass with large value. It is needed to avoid
isolating the output of transformer from the input, which would create a
situation that voltage between input and output would not be defined.

Main program:

```
#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	double t = 0.0;
	struct libsimul_ctx ctx;
	linsimul_init(&ctx, dt);
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
```

Plot of output:

![transformer plot](plots/transformerplot.png)

### True sine wave inverter

Circuit schematic:

![inverter schematic](figs/inverterpwm.png)

Netlist inverterpwm.txt:

```
1 0 V1 V=400 R=1e-3
1 2 S1 R=1e-3
1 3 S3 R=1e-3
0 2 S4 R=1e-3
0 3 S2 R=1e-3
2 4 L1 L=1020e-6 Iinit=0
4 5 R1 R=0.17745
5 3 C1 C=7.5e-6 R=1e-3
5 3 RL R=100
```

Program to control it:

```
#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

void set_switches(struct libsimul_ctx *ctx, int onoff)
{
	if (onoff)
	{
		set_switch_state(ctx, "S1", 1);
		set_switch_state(ctx, "S2", 1);
		set_switch_state(ctx, "S3", 0);
		set_switch_state(ctx, "S4", 0);
	}
	else
	{
		set_switch_state(ctx, "S1", 0);
		set_switch_state(ctx, "S2", 0);
		set_switch_state(ctx, "S3", 1);
		set_switch_state(ctx, "S4", 1);
	}
	recalc(ctx);
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
	struct libsimul_ctx ctx;
	linsimul_init(&ctx, dt);
	read_file(&ctx, "inverterpwm.txt");
	init_simulation(&ctx);
	set_switches(&ctx, 1);
	cnt_remain = 1;
	for (i = 0; i < 1000*1000; i++)
	{
		simulation_step(&ctx);
		t += dt;
		printf("%zu %g\n", i, get_V(&ctx, 5)-get_V(&ctx, 3));
		cnt_remain--;
		if (cnt_remain == 0 && onoff == 1)
		{
			set_switches(&tx, 0);
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
			set_switches(&ctx, 1);
			onoff = 1;
		}
	}
	return 0;
}
```

Plot of output:

![true sine wave PWM inverter plot](plots/inverterpwmplot.png)

### 3-phase true sine wave inverter

Circuit schematic:

![3-phase inverter schematic](figs/inverterpwm3.png)

Netlist inverterpwm3.txt:

```
1 0 V1 V=550 R=1e-3
1 2 S1 R=1e-3
0 2 S2 R=1e-3
1 3 S3 R=1e-3
0 3 S4 R=1e-3
1 4 S5 R=1e-3
0 4 S6 R=1e-3
2 5 L1 L=1020e-6 Iinit=0
2 5 RRL1 R=0.5e5
3 6 L2 L=1020e-6 Iinit=0
3 6 RRL2 R=0.5e5
4 7 L3 L=1020e-6 Iinit=0
4 7 RRL3 R=0.5e5
5 8 R1 R=0.17745
6 9 R2 R=0.17745
7 10 R3 R=0.17745
8 11 C1 C=7.5e-6 R=1e-3
9 11 C2 C=7.5e-6 R=1e-3
10 11 C3 C=7.5e-6 R=1e-3
8 9 RL1 R=300
9 10 RL2 R=300
10 8 RL3 R=300
1 12 S7 R=1e-3
0 12 S8 R=1e-3
12 13 L4 L=1020e-6 Iinit=0
12 13 RRL4 R=0.5e5
13 11 R4 R=0.17745
```

Program to control it:

```
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libsimul.h"

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
	else if (phase == 0)
	{
		S1 = "S7";
		S2 = "S8";
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
	int cnt_remain0 = 500;
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
	double ontime0 = 1e-5, offtime0 = 1e-5;
	double ontime1 = 1e-5, offtime1 = 1e-5, V_out_ideal;
	double ontime2 = 1e-5, offtime2 = 1e-5;
	double ontime3 = 1e-5, offtime3 = 1e-5;
	int onoff0 = 0, onoff1 = 0, onoff2 = 0, onoff3 = 0;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "inverterpwm3.txt");
	init_simulation(&ctx);
	set_switches(&ctx, 0, 1);
	set_switches(&ctx, 1, 1);
	set_switches(&ctx, 2, 1);
	set_switches(&ctx, 3, 1);
	cnt_remain0 = 1;
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
		cnt_remain0--;
		cnt_remain1--;
		cnt_remain2--;
		cnt_remain3--;
		if (cnt_remain0 == 0 && onoff0 == 1)
		{
			set_switches(&ctx, 0, 0);
			cnt_remain0 = offtime0/dt;
			onoff0 = 0;
		}
		else if (cnt_remain0 == 0 && onoff0 == 0)
		{
			ontime0 = 0.5/switch_f;
			offtime0 = 0.5/switch_f;
			cnt_remain0 = ontime0/dt;
			set_switches(&ctx, 0, 1);
			onoff0 = 1;
		}
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
```

Plot of output:

![3-phase true sine wave PWM inverter plot](plots/inverterpwmplot3.png)

### Forward converter

Circuit schematic:

![forward converter schematic](figs/forward.png)

Netlist forward.txt:

```
1 0 VS V=24 R=1e-3
1 2 X1 N=100 primary=1 Lbase=5e-7 R=6e-3
3 1 X1 N=50 primary=0 R=12e-3
2 0 S1 R=1e-3
0 3 D3 R=1e-3
0 3 RRD3 R=1e5
5 4 X1 N=50 primary=0 R=3e-3
0 4 Rbypass R=1e10
5 6 D1 R=1e-3
5 6 RRD1 R=1e10
4 6 D2 R=1e-3
6 7 RRL1 R=1e10
6 7 L1 L=1e-3
7 8 RL1 R=10e-3
8 4 C1 C=2200e-6 R=1e-3
8 4 RL R=24
```

Note the RRL1 which prevents isolation of node 6 and Rbypass which prevents
isolation of transformer primary and secondary sides. Note also the RRD3 which
is needed for the new transformer model to prevent recalculation loop, and the
RRD3 can't be `1e10` since the R-L circuit would have too short time constant,
so a value of `1e5` is needed. Note also `RRD1` with high value that is needed
for stabilizing the simulation.

Program to control it:

```
#include <stdio.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "forward.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 3*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 7) - get_V(&ctx, 4));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				recalc(&ctx);
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
	libsimul_free(&ctx);
	return 0;
}
```

Plot of output:

![forward converter plot](plots/forwardplot.png)

Note that similar to buck converter with same control algorithm, the forward
converter has massive overshoot.

### Forward converter, better control

Circuit schematic:

![forward converter schematic](figs/forward.png)

Program to control forward converter in a better manner:

```
#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	int cnt_on = 0;
	double L, C;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "forward.txt");
	init_simulation(&ctx);
	L = get_inductor(&ctx, "L1");
	C = get_capacitor(&ctx, "C1");
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		recalc(&ctx);
	}
	for (i = 0; i < 3*1000*1000; i++)
	{
		double V_out, I_ind, V_new;
		simulation_step(&ctx);
		V_out = get_V(&ctx, 7) - get_V(&ctx, 4);
		I_ind = get_inductor_current(&ctx, "L1");
		V_new = sqrt(V_out*V_out + L/C*I_ind*I_ind);
		printf("%zu %g\n", i, get_V(&ctx, 7) - get_V(&ctx, 4));
		cnt_remain--;
		if (switch_state && V_new > 24*0.5*0.5*1.003)
		{
			cnt_on++;
			cnt_remain = 0;
		}
		else if (switch_state)
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
				cnt_remain = 530;
				cnt_on = 0;
			}
			else
			{
				cnt_remain = 1000 - cnt_on;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
```

Plot of output with better control:

![forward converter plot, good control](plots/forwardgoodplot.png)

Now the control is perfect.

### Flyback converter

Circuit schematic:

![flyback converter schematic](figs/flyback.png)

Netlist flyback.txt:

```
1 0 VS V=24 R=1e-3
1 2 T1 N=100 primary=1 Lbase=1e-6 Vmin=-5000 Vmax=5000 R=6e-3
2 0 S1 R=1e-3
2 0 RRS1 R=1e10
3 4 T1 N=50 primary=0 R=3e-3
0 3 Rbypass R=1e10
4 5 D1 R=1e-3
5 3 C1 C=2200e-6 R=1e-3
5 3 RL R=24
```

Alternative netlist for newer fast transformer model:

```
1 0 VS V=24 R=1e-3
1 2 X1 N=100 primary=1 Lbase=1e-6 R=6e-3
2 0 S1 R=1e-3
3 4 X1 N=50 primary=0 R=3e-3
0 3 Rbypass R=1e10
4 5 D1 R=1e-3
5 3 C1 C=2200e-6 R=1e-3
5 3 RL R=24
```

Note the Rbypass which prevents isolation of transformer primary and secondary
sides. Note also RRS1 for bypassing the switch which is needed for the old
transformer model.

Program to control it:

```
#include <stdio.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "flyback.txt");
	init_simulation(&ctx);
	if (set_switch_state(&ctx, "S1", switch_state) != 0)
	{
		set_diode_hint(&ctx, "D1", !switch_state);
		recalc(&ctx);
	}
	for (i = 0; i < 3*1000*1000; i++)
	{
		simulation_step(&ctx);
		printf("%zu %g\n", i, get_V(&ctx, 5) - get_V(&ctx, 3));
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				set_diode_hint(&ctx, "D1", !switch_state);
				recalc(&ctx);
			}
			if (switch_state)
			{
				cnt_remain = 500;
			}
			else
			{
				cnt_remain = 100;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
```

Plot of output:

![flyback converter plot](plots/flybackplot.png)

Note the massive overshoot and slow oscillation. Note also the major increase
from input voltage: flyback converters can be used to create very high voltages
if needed.

### Flyback converter with physical model control

It is possible to implement better control for flyback converter.

Circuit schematic:

![flyback converter schematic](figs/flyback.png)

Netlist flybackgood.txt:

```
1 0 VS V=24 R=1e-3
1 2 X1 N=100 primary=1 Lbase=1e-6 Vmin=-5000 Vmax=5000 R=6e-3
2 0 S1 R=1e-3
3 4 X1 N=50 primary=0 R=3e-3
0 3 Rbypass R=1e10
4 5 D1 R=1e-3
5 3 C1 C=2200e-6 R=1e-3
5 3 RL R=24
```

Code:

```
#include <stdio.h>
#include <math.h>
#include "libsimul.h"

const double dt = 1e-7; // 100 ns

int main(int argc, char **argv)
{
	size_t i;
	int switch_state = 1;
	int cnt_remain = 500;
	int cnt_on = 0;
	struct libsimul_ctx ctx;
	double C, L, R;
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
		double E_switch = 60.0*60.0/R*1e-7*1000;
		double V_new = sqrt(V_out*V_out + L/C*I_xfr*I_xfr - 2*E_switch/C);
		printf("%zu %g %g\n", i, V_out, I_xfr);
		cnt_remain--;
		if (switch_state && (V_new > 60*1.03 || I_xfr > 15))
		{
			cnt_on++;
			cnt_remain = 0;
		}
		else if (switch_state)
		{
			cnt_on++;
		}
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			if (set_switch_state(&ctx, "S1", switch_state) != 0)
			{
				set_diode_hint(&ctx, "D1", !switch_state);
				recalc(&ctx);
			}
			if (switch_state)
			{
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
```

Plot of output:

![flyback converter plot with good control](plots/flybackgoodplot.png)

### Active power factor correction (PFC) circuit based on boost converter

Circuit schematic:

![power factor correction circuit schematic](figs/pfcboost.png)

Netlist pfcboost.txt:

```
# Sine voltage source, controlled from C code, initial value 0
1 0 V1 V=0 R=1e-3

# Full-wave rectifier, in:1,0; out:2,3
1 2 D1 R=1e-3 diode_threshold=1e-6
1 2 RD1 R=1e10
0 2 D2 R=1e-3 diode_threshold=1e-6
3 1 D3 R=1e-3 diode_threshold=1e-6
3 1 RD3 R=1e10
3 0 D4 R=1e-3 diode_threshold=1e-6

# Boost converter, in: 2,3; out: 6,3
2 4 RRL1 R=1e5
2 4 L1 L=10e-3
4 5 RL1 R=60e-3
5 3 S1 R=1e-3
5 6 D5 R=1e-3 diode_threshold=1e-6
6 3 C1 C=47e-6 R=1e-3
6 3 RL R=1000
```

Program to control it:

```
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
	int sign_last_nonzero = 0;
	const double V_tgt = 230*sqrt(2)*1.2;
	double C;
	struct libsimul_ctx ctx;
	libsimul_init(&ctx, dt);
	read_file(&ctx, "pfcboost.txt");
	init_simulation(&ctx);
	C = get_capacitor(&ctx, "C1");
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
		double I_ind = -get_inductor_current(&ctx, "L1");
		double V_rect = get_V(&ctx, 2) - get_V(&ctx, 3);
		double V_out = get_V(&ctx, 6) - get_V(&ctx, 3);
		printf("%zu %g (%d) %g %g %g\n", i, V_input, switch_state, V_out, V_rect, I_ind);
		if (my_signum(V_input) != 0)
		{
			if (sign_last_nonzero != my_signum(V_input))
			{
				double E_cap;
				double E_ideal;
				E_cap = 0.5*C*V_out*V_out;
				if (V_out < 230*sqrt(2))
				{
					// Avoid current surge at ramp-up
					E_cap = 0.5*C*230*230*2;
				}
				E_ideal = 0.5*C*V_tgt*V_tgt;
				I_diff_single = (E_ideal-E_cap)*2*50.0/230.0;
				if (V_out < 230*sqrt(2)*0.9)
				{
					I_diff_single = 0;
				}
				else
				{
					I_ideal_rms_230 += I_diff_single/20.0;
				}
				I_diff_single = 19.0/20.0*I_diff_single;
			}
			sign_last_nonzero = my_signum(V_input);
		}
		cnt_remain--;
		if (switch_state && I_ind > I_ideal + 0.01)
		{
			cnt_remain = 0;
		}
		else if (!switch_state && I_ind < I_ideal - 0.01)
		{
			cnt_remain = 0;
		}
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
				cnt_remain = 100000;
				cnt_on = 0;
			}
			else
			{
				cnt_remain = 100000;
			}
		}
	}
	libsimul_free(&ctx);
	return 0;
}
```

Plot of output voltage:

![output voltage of PFC circuit](plots/pfcboostvoltage.png)

Plot of inductor current:

![inductor current of PFC circuit](plots/pfcboostcurrent.png)

## License

All of the material related to RLCTrans is licensed under the following MIT
license:

Copyright (c) 2023-2024 Juha-Matti Tilli

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
