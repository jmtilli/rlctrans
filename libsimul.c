#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0
#include <lapack.h>
#else
#define lapack_int int
#define LAPACK_dgetrf dgetrf_
void LAPACK_dgetrf(const lapack_int*, const lapack_int*, double*, const lapack_int*, lapack_int*, lapack_int*);
#define LAPACK_dgetrs dgetrs_
void LAPACK_dgetrs(const char*, const lapack_int*, const lapack_int*, const double*, const lapack_int*, const lapack_int*, double*, const lapack_int*, lapack_int*);
#endif
#include <math.h>
#include <stdint.h>
#include "libsimul.h"

int iswhiteonly(const char *ln)
{
	for (;;)
	{
		if (*ln == '\0')
		{
			return 1;
		}
		if (*ln != ' ' && *ln != '\t')
		{
			return 0;
		}
		ln++;
	}
}

size_t spaceoff(const char *ln)
{
	const char *orig_ln = ln;
	for (;;)
	{
		if (*ln == '\0' || *ln == ' ' || *ln == '\t')
		{
			return ln - orig_ln;
		}
		ln++;
	}
}
size_t nonspaceoff(const char *ln)
{
	const char *orig_ln = ln;
	for (;;)
	{
		if (*ln == '\0' || (*ln != ' ' && *ln != '\t'))
		{
			return ln - orig_ln;
		}
		ln++;
	}
}

int getline_strip_comment(FILE *f, char **ln, size_t *lnsz)
{
	int ch;
	size_t off = 0;
	if (off+1 >= *lnsz || *ln == NULL)
	{
		char *newln;
		size_t newsz = 2*(off+1)+16;
		newln = realloc(*ln, newsz);
		if (newln == NULL)
		{
			return -ERR_NO_MEMORY;
		}
		*ln = newln;
		*lnsz = newsz;
	}
	(*ln)[off] = '\0';
	for (;;)
	{
		ch = getc(f);
		if (ch == EOF && off == 0) {
			return -ERR_NO_DATA;
		}
		if (ch == EOF || ch == '\n') {
			break;
		}
		if (ch == '#') {
			for (;;) {
				int ch2 = getc(f);
				if (ch2 == EOF || ch2 == '\n')
				{
					break;
				}
			}
			break;
		}
		if (off+1 >= *lnsz || *ln == NULL)
		{
			char *newln;
			size_t newsz = 2*(off+1)+16;
			newln = realloc(*ln, newsz);
			if (newln == NULL)
			{
				return -ERR_NO_MEMORY;
			}
			*ln = newln;
			*lnsz = newsz;
		}
		(*ln)[off++] = ch;
		(*ln)[off] = '\0';
	}
	return 0;
}

void set_voltage_source(struct libsimul_ctx *ctx, const char *vsname, double V)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, vsname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Voltage source %s not found\n", vsname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_VOLTAGE)
	{
		fprintf(stderr, "Element %s not a voltage source\n", vsname);
		exit(1);
	}
	ctx->elements_used[i]->V = V;
	ctx->elements_used[i]->I_src = V/ctx->elements_used[i]->R;
}
int set_inductor(struct libsimul_ctx *ctx, const char *indname, double L)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, indname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Inductor %s not found\n", indname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_INDUCTOR)
	{
		fprintf(stderr, "Element %s not an inductor\n", indname);
		exit(1);
	}
	ctx->elements_used[i]->L = L;
	return 0;
}
double get_inductor_current(struct libsimul_ctx *ctx, const char *indname)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, indname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Inductor %s not found\n", indname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_INDUCTOR)
	{
		fprintf(stderr, "Element %s not an inductor\n", indname);
		exit(1);
	}
	return ctx->elements_used[i]->I_src;
}
int set_resistor(struct libsimul_ctx *ctx, const char *rsname, double R)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, rsname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Resistor %s not found\n", rsname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_RESISTOR)
	{
		fprintf(stderr, "Element %s not a resistor\n", rsname);
		exit(1);
	}
	ctx->elements_used[i]->R = R;
	return ERR_HAVE_TO_SIMULATE_AGAIN;
}

int set_switch_state(struct libsimul_ctx *ctx, const char *swname, int state)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, swname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Switch %s not found\n", swname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_SWITCH)
	{
		fprintf(stderr, "Element %s not a switch\n", swname);
		exit(1);
	}
	if ((!!ctx->elements_used[i]->current_switch_state_is_closed) == (!!state))
	{
		return 0;
	}
	ctx->elements_used[i]->current_switch_state_is_closed = !!state;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (ctx->elements_used[i]->typ == TYPE_DIODE)
		{
			// FIXME spooky...
			// This seems to cause more problems than it solves.
			// At least pfc3 is negatively affected.
#if 0
			ctx->elements_used[i]->current_switch_state_is_closed = 1;
#endif
		}
	}
	return ERR_HAVE_TO_SIMULATE_AGAIN;
}
int set_diode_hint(struct libsimul_ctx *ctx, const char *dname, int state)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, dname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Diode %s not found\n", dname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_DIODE)
	{
		fprintf(stderr, "Element %s not a diode\n", dname);
		exit(1);
	}
	ctx->elements_used[i]->current_switch_state_is_closed = !!state;
	return ERR_HAVE_TO_SIMULATE_AGAIN;
}

void mark_node_seen(struct libsimul_ctx *ctx, int n)
{
	size_t nsz;
	size_t i;
	if (n < 0)
	{
		abort();
	}
	nsz = (size_t)n;
	if (ctx->node_seen == NULL || nsz >= ctx->node_seen_cap)
	{
		size_t newcap = nsz*2 + 16;
		unsigned char *new_ns;
		new_ns = realloc(ctx->node_seen, sizeof(*ctx->node_seen)*newcap);
		if (new_ns == NULL)
		{
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		ctx->node_seen = new_ns;
		ctx->node_seen_cap = newcap;
		for (i = ctx->node_seen_sz; i < newcap; i++)
		{
			new_ns[i] = 0;
		}
	}
	ctx->node_seen[nsz] = 1;
	if (nsz+1 > ctx->node_seen_sz)
	{
		ctx->node_seen_sz = nsz+1;
	}
}
void check_dense_nodes(struct libsimul_ctx *ctx)
{
	size_t i;
	if (ctx->node_seen_sz < 2)
	{
		fprintf(stderr, "Must have at least 2 nodes\n");
		exit(1);
	}
	for (i = 0; i < ctx->node_seen_sz; i++)
	{
		if (!ctx->node_seen[i])
		{
			fprintf(stderr, "Node %zu not seen\n", i);
			exit(1);
		}
	}
}

void form_g_matrix(struct libsimul_ctx *ctx)
{
	struct element *el;
	const size_t nodecnt = ctx->nodecnt;
	size_t x,y;
	size_t i;
	for (x = 0; x < nodecnt; x++)
	{
		for (y = 0; y < nodecnt; y++)
		{
			// Column major
			ctx->G_matrix[x*nodecnt+y] = 0;
		}
	}
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		int n1;
		int n2;
		double G;
		el = ctx->elements_used[i];
		if (el->typ == TYPE_INDUCTOR)
		{
			continue;
		}
		if (((el->typ == TYPE_DIODE || el->typ == TYPE_SWITCH) &&
		     !el->current_switch_state_is_closed))
		{
			G = 1e-9; // FIXME this is bad.
			continue;
		}
		else if (el->typ == TYPE_SHOCKLEY_DIODE)
		{
			double V = get_V(ctx, el->n1) - get_V(ctx, el->n2);
			double I_model, V_across_resistor;
			I_model = V*el->G_R_shockley - el->I_src;
			V_across_resistor = el->R*I_model;
			V -= V_across_resistor;
			if (V < el->V_across_diode - 0.1 && V > 0)
			{
				V = el->V_across_diode - 0.1;
			}
			else if (V > el->V_across_diode + 0.1 && V > 0)
			{
				//printf("V was %g\n", V);
				V = el->V_across_diode + 0.1;
				if (V < 0)
				{
					V = 0;
				}
				//printf("Setting V to %g\n", V);
			}
#if 1
			if (V > el->Vmax)
			{
				//printf("Vmax hit %g %g %g\n", el->V_across_diode, V, el->Vmax);
				V = el->Vmax;
			}
#endif
			el->expval = exp(V/el->V_T);
			el->I_model = I_model;
			G = el->I_s/el->V_T*el->expval;
			el->G_shockley = G; // not including resistance
			G = 1.0/(1.0/G + el->R);
			el->G_R_shockley = G; // including resistance
		}
		else
		{
			G = 1.0/el->R;
		}
		n1 = el->n1;
		n2 = el->n2;
		if (n1 != 0)
		{
			ctx->G_matrix[(n1-1)*nodecnt+(n1-1)] += G;
		}
		if (n2 != 0)
		{
			ctx->G_matrix[(n2-1)*nodecnt+(n2-1)] += G;
		}
		if (n1 != 0 && n2 != 0)
		{
			ctx->G_matrix[(n1-1)*nodecnt+(n2-1)] -= G;
			ctx->G_matrix[(n2-1)*nodecnt+(n1-1)] -= G;
		}
	}
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		size_t j,k;
		el = ctx->elements_used[i];
		if (el->typ != TYPE_TRANSFORMER_DIRECT || !el->primary)
		{
			continue;
		}
		for (j = 0; j < el->allptrs_size; j++)
		{
			for (k = 0; k < el->allptrs_size; k++)
			{
				struct element *thiselement = el->allptrs[j];
				struct element *thatelement = el->allptrs[k];
				int this1 = thiselement->n1;
				int this2 = thiselement->n2;
				int that1 = thatelement->n1;
				int that2 = thatelement->n2;
				double G = 
					(1.0/thatelement->R/thiselement->R)*thatelement->N*thiselement->N
					/ (el->N * el->N * el->transformer_direct_denom);
				G = -G; // This is necessary!
				if (this1 != 0 && that1 != 0)
				{
					ctx->G_matrix[(this1-1)*nodecnt+(that1-1)] += G;
				}
				if (this2 != 0 && that2 != 0)
				{
					ctx->G_matrix[(this2-1)*nodecnt+(that2-1)] += G;
				}
				if (this1 != 0 && that2 != 0)
				{
					ctx->G_matrix[(this1-1)*nodecnt+(that2-1)] -= G;
				}
				if (this2 != 0 && that1 != 0)
				{
					ctx->G_matrix[(this2-1)*nodecnt+(that1-1)] -= G;
				}
			}
		}
	}
}
void calc_lu(struct libsimul_ctx *ctx)
{
	const size_t nodecnt = ctx->nodecnt;
	const int n = nodecnt;
	int info = 0;
	memcpy(ctx->G_LU, ctx->G_matrix, sizeof(*ctx->G_LU)*nodecnt*nodecnt);
	LAPACK_dgetrf(&n, &n, ctx->G_LU, &n, ctx->G_ipiv, &info);
	if (info != 0)
	{
		fprintf(stderr, "Can't LU decompose: %d\n", info);
		exit(1);
	}
}
void calc_V(struct libsimul_ctx *ctx)
{
	const size_t nodecnt = ctx->nodecnt;
	const int n = nodecnt;
	const int one = 1;
	int info = 0;
	memcpy(ctx->V_vector, ctx->Isrc_vector, sizeof(*ctx->V_vector)*nodecnt);
	LAPACK_dgetrs("N", &n, &one, ctx->G_LU, &n, ctx->G_ipiv, ctx->V_vector, &n, &info);
	if (info != 0)
	{
		fprintf(stderr, "Can't solve system of equations\n");
		exit(1);
	}
}
void form_isrc_vector(struct libsimul_ctx *ctx)
{
	const size_t nodecnt = ctx->nodecnt;
	const size_t elements_used_sz = ctx->elements_used_sz;
	struct element *el;
	size_t i;
	size_t x;
	for (x = 0; x < nodecnt; x++)
	{
		ctx->Isrc_vector[x] = 0;
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		int n1;
		int n2;
		double Isrc;
		el = ctx->elements_used[i];
		if (el->typ == TYPE_TRANSFORMER_DIRECT)
		{
			continue;
		}
		else if (el->typ == TYPE_SHOCKLEY_DIODE)
		{
			double V = get_V(ctx, el->n1) - get_V(ctx, el->n2);
			double I_model, V_across_resistor;
			I_model = el->I_model;
			//I_model = V*el->G_R_shockley - el->I_src;
			V_across_resistor = el->R*I_model;
			V -= V_across_resistor;
			if (V < el->V_across_diode - 0.1 && V > 0)
			{
				V = el->V_across_diode - 0.1;
			}
			else if (V > el->V_across_diode + 0.1 && V > 0)
			{
				//printf("V was %g\n", V);
				V = el->V_across_diode + 0.1;
				if (V < 0)
				{
					V = 0;
				}
				//printf("Setting V to %g\n", V);
			}
#if 1
			if (V > el->Vmax)
			{
				//printf("Vmax hit %g %g %g\n", el->V_across_diode, V, el->Vmax);
				V = el->Vmax;
			}
#endif
			//el->expval = exp(V/el->V_T); // already calculated
			Isrc = el->I_s*(1+(V/el->V_T-1)*el->expval);
			// Isrc and el->G_shockley in parallel, el->R in series
			// converted to, in series:
			// 1. voltage Isrc/el->G_shockley
			// 2. resistor 1.0/el->G_shockley
			// 3. resistor el->R
			// converted to, in series:
			// 1. voltage Isrc/el->G_shockley
			// 2. resistor 1.0/el->G_shockley + el->R
			// converted to, in parallel:
			// 1. current src Isrc*el->G_R_shockley/el->G_shockley
			// 2. conductance el->G_R_shockley
			// Here (2) is el->G_R_shockley
			if (el->G_shockley != 0)
			{
				Isrc *= el->G_R_shockley/el->G_shockley;
			}
			el->I_src = Isrc; // including resistance
		}
		else
		{
			Isrc = el->I_src;
		}
		n1 = el->n1;
		n2 = el->n2;
		if (n1 != 0)
		{
			ctx->Isrc_vector[n1-1] += Isrc;
		}
		if (n2 != 0)
		{
			ctx->Isrc_vector[n2-1] -= Isrc;
		}
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		size_t j;
		el = ctx->elements_used[i];
		if (el->typ != TYPE_TRANSFORMER_DIRECT || !el->primary)
		{
			continue;
		}
		for (j = 0; j < el->allptrs_size; j++)
		{
			struct element *winding = el->allptrs[j];
			int n1 = winding->n1;
			int n2 = winding->n2;
			double Isrc =
				winding->N/el->N * 1.0/winding->R *
				el->transformer_direct_const
				/ el->transformer_direct_denom;
			if (n1 != 0)
			{
				ctx->Isrc_vector[n1-1] += Isrc;
			}
			if (n2 != 0)
			{
				ctx->Isrc_vector[n2-1] -= Isrc;
			}
		}
	}
}

double get_V(struct libsimul_ctx *ctx, int node)
{
	if (node == 0)
	{
		return 0;
	}
	return ctx->V_vector[node-1];
}

int go_through_shockley_diodes(struct libsimul_ctx *ctx)
{
	const size_t elements_used_sz = ctx->elements_used_sz;
	size_t i;
	double V_across_diode;
	double V_across_resistor;
	double I_linear, I_nonlinear;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_SHOCKLEY_DIODE)
		{
			continue;
		}
		//printf("Found Shockley diode\n");
		V_across_diode = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		I_linear = V_across_diode*el->G_R_shockley - el->I_src;
		V_across_resistor = el->R*I_linear;
		V_across_diode -= V_across_resistor;
		if (V_across_diode < 0)
		{
			el->V_across_diode = V_across_diode;
		}
		else if (V_across_diode > el->V_across_diode + 0.1)
		{
			el->V_across_diode += 0.1;
			if (el->V_across_diode < 0)
			{
				el->V_across_diode = 0;
			}
		}
		else if (V_across_diode < el->V_across_diode - 0.1)
		{
			el->V_across_diode -= 0.1;
		}
		else
		{
			el->V_across_diode = V_across_diode;
		}
		I_nonlinear = el->I_s*(exp(V_across_diode/el->V_T)-1);
		//printf("V_across_diode %g\n", V_across_diode);
		//printf("I_nonlinear %g\n", I_nonlinear);
		//printf("I_linear %g\n", I_linear);
		if (fabs(I_nonlinear - I_linear) > el->I_accuracy)
		{
			//printf("Shockley iter\n");
			return ERR_HAVE_TO_SIMULATE_AGAIN_SHOCKLEY_DIODE;
		}
	}
	return 0;
}
int go_through_shockley_diodes_2(struct libsimul_ctx *ctx)
{
	const size_t elements_used_sz = ctx->elements_used_sz;
	size_t i;
	double V_across_diode;
	double V_across_resistor;
	double I_model;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_SHOCKLEY_DIODE)
		{
			continue;
		}
		//printf("Found Shockley diode 2\n");
		V_across_diode = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		I_model = V_across_diode*el->G_R_shockley - el->I_src;
		V_across_resistor = el->R*I_model;
		V_across_diode -= V_across_resistor;
		el->V_across_diode = V_across_diode;
	}
	return 0;
}

// Return: 0 OK
// Return: -EAGAIN have to do simulation again
int go_through_diodes(struct libsimul_ctx *ctx, int recalc_loop)
{
	const size_t elements_used_sz = ctx->elements_used_sz;
	size_t i;
	int ret = 0;
	double V_across_diode;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_DIODE)
		{
			continue;
		}
		if (recalc_loop && el->on_recalc != -1)
		{
			if (el->current_switch_state_is_closed != el->on_recalc)
			{
				ret = ERR_HAVE_TO_SIMULATE_AGAIN_DIODE;
			}
			el->current_switch_state_is_closed = el->on_recalc;
			continue;
		}
		V_across_diode = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		if (V_across_diode < -el->diode_threshold && el->current_switch_state_is_closed)
		{
			el->current_switch_state_is_closed = 0;
			ret = ERR_HAVE_TO_SIMULATE_AGAIN_DIODE;
		}
		if (V_across_diode > el->diode_threshold && !el->current_switch_state_is_closed)
		{
			el->current_switch_state_is_closed = 1;
			ret = ERR_HAVE_TO_SIMULATE_AGAIN_DIODE;
		}
	}
	return ret;
}
int signum(double d)
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
void go_through_inductors(struct libsimul_ctx *ctx)
{
	size_t i;
	double V_across_inductor;
	double V_across_resistor;
	double dI;
	int oldsign, newsign;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_INDUCTOR)
		{
			continue;
		}
		V_across_inductor = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		V_across_resistor = -el->R*el->I_src;
		V_across_inductor -= V_across_resistor;
		dI = -V_across_inductor/el->L*ctx->dt;
#if 0
		if (fabs(V_across_inductor) > 100)
		{
			fprintf(stderr, "Limit1 reached\n");
			exit(1);
		}
#endif
		oldsign = signum(el->I_src);
		el->I_src += dI;
		newsign = signum(el->I_src);
		if (oldsign != 0 && newsign != 0 && oldsign != newsign)
		{
			// Probably wisest to reset to zero
			el->I_src = 0;
		}
#if 0
		if (fabs(el->I_src) > 100)
		{
			fprintf(stderr, "Limit2 reached\n");
			exit(1);
		}
#endif
		//fprintf(stderr, "Inductor V_across %g I_src %g\n", V_across_inductor, el->I_src);
	}
}
void go_through_capacitors(struct libsimul_ctx *ctx)
{
	size_t i;
	double V_across_capacitor;
	double I_R;
	double I_tot;
	double dU;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_CAPACITOR)
		{
			continue;
		}
		V_across_capacitor = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		I_R = V_across_capacitor/el->R;
		I_tot = el->I_src - I_R;
		// Positive I_tot: discharge
		// Negative I_tot: charge
		dU = -I_tot/el->C*ctx->dt;
		el->I_src += dU/el->R;
	}
}
double get_transformer_dphi_single(struct libsimul_ctx *ctx, size_t el_id)
{
	double V_trial;
	V_trial = ctx->elements_used[el_id]->R * ctx->elements_used[el_id]->I_src;
	return V_trial*ctx->dt/ctx->elements_used[el_id]->N;
}
void go_through_transformers(struct libsimul_ctx *ctx)
{
	size_t i;
	int oldsign, newsign;
	double dphi_single;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_TRANSFORMER || !el->primary)
		{
			continue;
		}
		dphi_single = get_transformer_dphi_single(ctx, i);
		oldsign = signum(el->cur_phi_single);
		el->cur_phi_single += dphi_single;
		newsign = signum(el->cur_phi_single);
		if (oldsign != 0 && newsign != 0 && oldsign != newsign)
		{
			// Probably wisest to reset to zero
			el->cur_phi_single = 0;
		}
	}
}
double get_transformer_direct_dconst(struct libsimul_ctx *ctx, size_t i)
{
	struct element *el = ctx->elements_used[i];
	const double Const = el->transformer_direct_const;
	const double denom = el->transformer_direct_denom;
	//denom = (N_3/N_1*N_3/N_1*1/R_3 + N_2/N_1*N_2/N_1*1/R_2 + N_1/N_1*N_1/N_1*1/R_1)
	//double V_1 = {Const + (V_nc1-V_nc2)/R_3*N_3/N_1 + (V_nb1-V_nb2)/R_2*N_2/N_1 + (V_na1-V_na2)/R_1*N_1/N_1}/denom;
	double V_1;
	size_t j;
	V_1 = Const;
	for (j = 0; j < el->allptrs_size; j++)
	{
		struct element *winding = el->allptrs[j];
		double contribution = (get_V(ctx, winding->n1) - get_V(ctx, winding->n2))/winding->R*winding->N/el->N;
		V_1 += contribution;
	}
	V_1 /= denom;
	return V_1*ctx->dt/(el->Lbase*el->N*el->N);

}
double get_transformer_mag_current(struct libsimul_ctx *ctx, const char *xfrname)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, xfrname) == 0 && ctx->elements_used[i]->primary)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Transformer %s not found\n", xfrname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER_DIRECT)
	{
		return -ctx->elements_used[i]->transformer_direct_const;
	}
	else if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER)
	{
		return ctx->elements_used[i]->cur_phi_single
		       / ctx->elements_used[i]->Lbase
		       / ctx->elements_used[i]->N;
	}
	else
	{
		fprintf(stderr, "Element %s not a transformer\n", xfrname);
		exit(1);
	}
}
double get_transformer_inductor(struct libsimul_ctx *ctx, const char *xfrname)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, xfrname) == 0 && ctx->elements_used[i]->primary)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Transformer %s not found\n", xfrname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER_DIRECT)
	{
		return ctx->elements_used[i]->Lbase
		       * ctx->elements_used[i]->N
		       * ctx->elements_used[i]->N;
	}
	else if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER)
	{
		return ctx->elements_used[i]->Lbase
		       * ctx->elements_used[i]->N
		       * ctx->elements_used[i]->N;
	}
	else
	{
		fprintf(stderr, "Element %s not a transformer\n", xfrname);
		exit(1);
	}
}
void go_through_direct_transformers(struct libsimul_ctx *ctx)
{
	size_t i;
	int oldsign, newsign;
	double dconst;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		struct element *el = ctx->elements_used[i];
		if (el->typ != TYPE_TRANSFORMER_DIRECT || !el->primary)
		{
			continue;
		}
		dconst = get_transformer_direct_dconst(ctx, i);
		oldsign = signum(el->transformer_direct_const);
		el->transformer_direct_const -= dconst;
		newsign = signum(el->transformer_direct_const);
		if (oldsign != 0 && newsign != 0 && oldsign != newsign)
		{
			// Probably wisest to reset to zero
			el->transformer_direct_const = 0;
		}
	}
}

void set_transformer_voltage(struct libsimul_ctx *ctx, size_t el_id, double V)
{
	size_t i;
#if 0
	ctx->elements_used[el_id]->I_src =
		V /
		ctx->elements_used[el_id]->R;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = elements_used[i];
		if (el->typ == TYPE_TRANSFORMER &&
		    !el->primary &&
		    el->primaryptr == elements_used[el_id])
		{
			el->I_src = 
				V / el->R * el->N / elements_used[el_id]->N;
		}
	}
#else
	for (i = 0; i < ctx->elements_used[el_id]->allptrs_size; i++)
	{
		struct element *el = ctx->elements_used[el_id]->allptrs[i];
		el->I_src = 
			V / el->R * el->N / ctx->elements_used[el_id]->N;
	}
#endif
}

double get_transformer_trial_phi_single(struct libsimul_ctx *ctx, size_t el_id)
{
	size_t i;
	double V_across_winding;
	double I_R;
	double I_tot;
	double phi_single = 0;
#if 0
	V_across_winding =
		get_V(ctx, ctx->elements_used[el_id]->n1) -
		get_V(ctx, ctx->elements_used[el_id]->n2);
	I_R = V_across_winding/ctx->elements_used[el_id]->R;
	I_tot = ctx->elements_used[el_id]->I_src - I_R;
	phi_single -=
		I_tot * 
		ctx->elements_used[el_id]->Lbase *
		ctx->elements_used[el_id]->N;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = elements_used[i];
		if (el->typ == TYPE_TRANSFORMER &&
		    !el->primary &&
		    el->primaryptr == elements_used[el_id])
		{
			V_across_winding = get_V(el->n1) - get_V(el->n2);
			I_R = V_across_winding/el->R;
			I_tot = el->I_src - I_R;
			phi_single -=
				I_tot * elements_used[el_id]->Lbase * el->N;
		}
	}
#else
	for (i = 0; i < ctx->elements_used[el_id]->allptrs_size; i++)
	{
		struct element *el = ctx->elements_used[el_id]->allptrs[i];
		V_across_winding = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		I_R = V_across_winding/el->R;
		I_tot = el->I_src - I_R;
		phi_single -=
			I_tot * ctx->elements_used[el_id]->Lbase * el->N;
	}
#endif
	return phi_single;
}

int double_cmp(double a, double b)
{
	if (a < b)
	{
		return -1;
	}
	if (a > b)
	{
		return 1;
	}
	return 0;
}

int go_through_all(struct libsimul_ctx *ctx, int recalc_loop)
{
	int ret = 0;
	ret = go_through_shockley_diodes(ctx);
	if (ret != 0)
	{
		return ret;
	}
	ret = go_through_diodes(ctx, recalc_loop);
	if (ret != 0)
	{
		return ret;
	}
	if (ctx->xformerstate == STATE_FINI && ctx->xformerid == SIZE_MAX)
	{
		size_t i;
		for (i = 0; i < ctx->elements_used_sz; i++)
		{
			struct element *el = ctx->elements_used[i];
			if (el->typ == TYPE_TRANSFORMER && el->primary)
			{
				break;
			}
		}
		ctx->xformerid = i;
		if (ctx->xformerid < ctx->elements_used_sz)
		{
			ctx->xformerstate = STATE_LOBO;
		}
	}
	else if (ctx->xformerstate == STATE_FINI && ctx->xformerid < ctx->elements_used_sz)
	{
		size_t i;
		for (i = ctx->xformerid+1; i < ctx->elements_used_sz; i++)
		{
			struct element *el = ctx->elements_used[i];
			if (el->typ == TYPE_TRANSFORMER && el->primary)
			{
				break;
			}
		}
		ctx->xformerid = i;
		if (ctx->xformerid < ctx->elements_used_sz)
		{
			ctx->xformerstate = STATE_LOBO;
		}
	}
	if (ctx->xformerstate == STATE_LOBO)
	{
		ctx->loboV = ctx->elements_used[ctx->xformerid]->Vmin;
		set_transformer_voltage(ctx, ctx->xformerid, ctx->loboV);
		ctx->xformerstate = STATE_LOBOPOST;
		return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
	}
	if (ctx->xformerstate == STATE_LOBOPOST)
	{
		int l;
		ctx->lobophi = get_transformer_trial_phi_single(ctx, ctx->xformerid);
		l = double_cmp(ctx->lobophi, ctx->elements_used[ctx->xformerid]->cur_phi_single);
		if (l == 0)
		{
			ctx->trialV = ctx->loboV;
			//set_transformer_voltage(ctx, ctx->xformerid, trialV);
			ctx->xformerstate = STATE_FINI;
			return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
		}
		ctx->xformerstate = STATE_HIBO;
	}
	if (ctx->xformerstate == STATE_HIBO)
	{
		ctx->hiboV = ctx->elements_used[ctx->xformerid]->Vmax;
		set_transformer_voltage(ctx, ctx->xformerid, ctx->hiboV);
		ctx->xformerstate = STATE_HIBOPOST;
		return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
	}
	if (ctx->xformerstate == STATE_HIBOPOST)
	{
		int l, h;
		ctx->hibophi = get_transformer_trial_phi_single(ctx, ctx->xformerid);
		l = double_cmp(ctx->lobophi, ctx->elements_used[ctx->xformerid]->cur_phi_single);
		h = double_cmp(ctx->hibophi, ctx->elements_used[ctx->xformerid]->cur_phi_single);
		if (h == 0)
		{
			ctx->trialV = ctx->hiboV;
			//set_transformer_voltage(ctx->xformerid, trialV);
			ctx->xformerstate = STATE_FINI;
			return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
		}
		if (l == h)
		{
			const double arbitrary_threshold = 1e-12;
			const double cur_phi_single =
				ctx->elements_used[ctx->xformerid]->cur_phi_single;
			if (fabs(cur_phi_single) < arbitrary_threshold)
			{
				if (fabs(ctx->lobophi) < arbitrary_threshold ||
				    fabs(ctx->hibophi) < arbitrary_threshold)
				{
					if (fabs(ctx->lobophi) <
					    fabs(ctx->hibophi))
					{
						ctx->trialV = ctx->loboV;
					}
					else
					{
						ctx->trialV = ctx->hiboV;
					}
					ctx->xformerstate = STATE_FINI;
					return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
				}
			}
			fprintf(stderr, "Cur phi single: %g\n", ctx->elements_used[ctx->xformerid]->cur_phi_single);
			fprintf(stderr, "Lobo phi: %g\n", ctx->lobophi);
			fprintf(stderr, "Hibo phi: %g\n", ctx->hibophi);
			fprintf(stderr, "Transformer out of voltage bounds\n");
			exit(1);
		}
		ctx->xformerstate = STATE_ITER;
	}
	if (ctx->xformerstate == STATE_ITERPOST)
	{
		int l, h, t;
		double iterphi;
		iterphi = get_transformer_trial_phi_single(ctx, ctx->xformerid);
		l = double_cmp(ctx->lobophi, ctx->elements_used[ctx->xformerid]->cur_phi_single);
		h = double_cmp(ctx->hibophi, ctx->elements_used[ctx->xformerid]->cur_phi_single);
		t = double_cmp(iterphi, ctx->elements_used[ctx->xformerid]->cur_phi_single);
		if (fabs(ctx->hiboV - ctx->loboV) < 1e-9)
		{
			//set_transformer_voltage(ctx, ctx->xformerid, trialV);
			ctx->xformerstate = STATE_FINI;
			return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
		}
		if (t == 0)
		{
			//set_transformer_voltage(ctx, ctx->xformerid, trialV);
			ctx->xformerstate = STATE_FINI;
			return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
		}
		if (l < 0 && h > 0)
		{
			if (t < 0)
			{
				ctx->loboV = ctx->trialV;
			}
			else if (t > 0)
			{
				ctx->hiboV = ctx->trialV;
			}
			else
			{
				abort();
			}
		}
		else if (l > 0 && h < 0)
		{
			if (t < 0)
			{
				ctx->hiboV = ctx->trialV;
			}
			else if (t > 0)
			{
				ctx->loboV = ctx->trialV;
			}
			else
			{
				abort();
			}
		}
		else
		{
			abort();
		}
		ctx->xformerstate = STATE_ITER;
	}
	if (ctx->xformerstate == STATE_ITER)
	{
		ctx->trialV = (ctx->loboV+ctx->hiboV)/2;
		set_transformer_voltage(ctx, ctx->xformerid, ctx->trialV);
		ctx->xformerstate = STATE_ITERPOST;
		return ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER;
	}
	go_through_inductors(ctx);
	go_through_capacitors(ctx);
	go_through_transformers(ctx);
	go_through_direct_transformers(ctx);
	ctx->xformerid = SIZE_MAX;
	ctx->xformerstate = STATE_FINI;
	return 0;
}

int add_element_used(struct libsimul_ctx *ctx, const char *element, int n1, int n2, enum element_type typ,
	double V,
	double Vinit,
	double Iinit,
	double L,
	double R,
	double C,
	double N,
	double Vmin,
	double Vmax,
	double Lbase,
	int primary,
	double diode_threshold,
	int on_recalc,
	double VT,
	double Is,
	double Iaccuracy)
{
	size_t i;
	struct element *el;

	if (typ == TYPE_INDUCTOR && L <= 0)
	{
		fprintf(stderr, "Inductor %s must have inductance\n", element);
		exit(1);
	}
	if (typ == TYPE_CAPACITOR && C <= 0)
	{
		fprintf(stderr, "Capacitor %s must have capacitance\n", element);
		exit(1);
	}
	if (typ == TYPE_SHOCKLEY_DIODE && Is <= 0)
	{
		fprintf(stderr, "Shockley diode %s must have saturation current\n", element);
		exit(1);
	}
	if (typ == TYPE_SHOCKLEY_DIODE && VT <= 0)
	{
		fprintf(stderr, "Shockley diode %s must have thermal voltage\n", element);
		exit(1);
	}
	if ((typ != TYPE_INDUCTOR && typ != TYPE_SHOCKLEY_DIODE) && R <= 0)
	{
		fprintf(stderr, "Non-inductor %s must have resistance\n", element);
		exit(1);
	}

	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, element) == 0)
		{
			if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER && typ == TYPE_TRANSFORMER && !(ctx->elements_used[i]->primary && primary))
			{
				continue;
			}
			if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER_DIRECT && typ == TYPE_TRANSFORMER_DIRECT && !(ctx->elements_used[i]->primary && primary))
			{
				continue;
			}
			fprintf(stderr, "Element %s already used\n", element);
			exit(1);
		}
	}
	if (ctx->elements_used_sz >= ctx->elements_used_cap || ctx->elements_used == NULL)
	{
		struct element **new_eu;
		size_t new_cap = 2*ctx->elements_used_sz+16;
		new_eu = realloc(ctx->elements_used, sizeof(*ctx->elements_used)*new_cap);
		if (new_eu == NULL)
		{
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		ctx->elements_used = new_eu;
		ctx->elements_used_cap = new_cap;
	}
	el = malloc(sizeof(**ctx->elements_used));
	ctx->elements_used[ctx->elements_used_sz] = el;
	el->name = strdup(element);
	el->n1 = n1;
	el->n2 = n2;
	el->typ = typ;
	el->V = V;
	el->Vinit = Vinit;
	el->Iinit = Iinit;
	el->L = L;
	el->R = R;
	el->C = C;
	el->I_src = 0;
	el->N = N;
	el->Vmin = Vmin;
	el->Vmax = Vmax;
	el->Lbase = Lbase;
	el->primary = primary;
	el->primaryptr = NULL;
	el->allptrs = NULL;
	el->allptrs_size = 0;
	el->allptrs_capacity = 0;
	el->cur_phi_single = 0;
	el->dphi_single = 0;
	el->transformer_direct_denom = 0;
	el->transformer_direct_const = 0;
	el->current_switch_state_is_closed = 1;
	el->diode_threshold = diode_threshold;
	el->on_recalc = on_recalc;
	el->I_s = Is;
	el->I_accuracy = Iaccuracy;
	el->V_T = VT;
	el->V_across_diode = 0;
	if (typ == TYPE_VOLTAGE)
	{
		el->I_src = V/R;
	}
	if (typ == TYPE_CAPACITOR)
	{
		el->I_src = Vinit/R;
	}
	if (typ == TYPE_INDUCTOR)
	{
		el->I_src = Iinit;
	}
	ctx->elements_used_sz++;
	if (typ == TYPE_SHOCKLEY_DIODE)
	{
		ctx->has_shockley = 1;
	}
	return 0;
}

void check_at_most_one_transformer(struct libsimul_ctx *ctx)
{
	const size_t elements_used_sz = ctx->elements_used_sz;
	size_t i;
	size_t j;
	int cnt = 0;
	for (i = 0; i < elements_used_sz; i++)
	{
		if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER && ctx->elements_used[i]->primary)
		{
			cnt++;
		}
	}
	if (cnt > 1)
	{
		fprintf(stderr, "Can have at most one transformer.\n");
		exit(1);
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER)
		{
			for (j = 0; j < elements_used_sz; j++)
			{
				if (ctx->elements_used[j]->typ == TYPE_TRANSFORMER && ctx->elements_used[j]->primary && strcmp(ctx->elements_used[i]->name, ctx->elements_used[j]->name) == 0)
				{
					ctx->elements_used[i]->primaryptr = ctx->elements_used[j];
					if (ctx->elements_used[j]->allptrs == NULL || ctx->elements_used[j]->allptrs_size >= ctx->elements_used[j]->allptrs_capacity)
					{
						struct element **sec2;
						size_t new_cap = ctx->elements_used[j]->allptrs_size*2+16;
						sec2 = realloc(ctx->elements_used[j]->allptrs, sizeof(*sec2)*new_cap);
						if (sec2 == NULL)
						{
							fprintf(stderr, "Out of memory\n");
							exit(1);
						}
						ctx->elements_used[j]->allptrs = sec2;
					}
					ctx->elements_used[j]->allptrs[ctx->elements_used[j]->allptrs_size++] = ctx->elements_used[i];
				}
			}
		}
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER_DIRECT)
		{
			for (j = 0; j < elements_used_sz; j++)
			{
				if (ctx->elements_used[j]->typ == TYPE_TRANSFORMER_DIRECT && ctx->elements_used[j]->primary && strcmp(ctx->elements_used[i]->name, ctx->elements_used[j]->name) == 0)
				{
					ctx->elements_used[i]->primaryptr = ctx->elements_used[j];
					if (ctx->elements_used[j]->allptrs == NULL || ctx->elements_used[j]->allptrs_size >= ctx->elements_used[j]->allptrs_capacity)
					{
						struct element **sec2;
						size_t new_cap = ctx->elements_used[j]->allptrs_size*2+16;
						sec2 = realloc(ctx->elements_used[j]->allptrs, sizeof(*sec2)*new_cap);
						if (sec2 == NULL)
						{
							fprintf(stderr, "Out of memory\n");
							exit(1);
						}
						ctx->elements_used[j]->allptrs = sec2;
					}
					ctx->elements_used[j]->allptrs[ctx->elements_used[j]->allptrs_size++] = ctx->elements_used[i];
				}
			}
		}
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		if (ctx->elements_used[i]->typ == TYPE_TRANSFORMER_DIRECT && ctx->elements_used[i]->primary)
		{
			struct element *primary = ctx->elements_used[i];
			for (j = 0; j < ctx->elements_used[i]->allptrs_size; j++)
			{
				struct element *winding = ctx->elements_used[i]->allptrs[j];
				// For three-winding transformer:
				// denom = (N_3/N_1*N_3/N_1*1/R_3 + N_2/N_1*N_2/N_1*1/R_2 + N_1/N_1*N_1/N_1*1/R_1)
				primary->transformer_direct_denom +=
					(winding->N/primary->N)*(winding->N/primary->N)*1.0/winding->R;
			}
		}
	}
}

void read_file(struct libsimul_ctx *ctx, const char *fname)
{
	char *line = NULL;
	size_t linesz = 0;
	FILE *f;
	f = fopen(fname, "r");
	if (f == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", fname);
		exit(1);
	}
	while (!feof(f)) {
		int ret;
		int has_more;
		long ln1, ln2;
		enum element_type typ;
		int n1, n2;
		char *first, *second, *third, *endptr;
		char *lineptr;
		size_t sp;
		int has_voltage = 0;
		double R = 0;
		double V = 0;
		int has_vmin = 0, has_vmax = 0;
		double L = 0;
		double C = 0;
		double Vinit = 0;
		double Iinit = 0;
		double N = 0;
		double Vmin = 0;
		double Vmax = 0;
		int primary = 0;
		int on_recalc = -1;
		double Lbase = 0;
		double diode_threshold = 0;
		double Is = 1e-12;
		double VT = 26e-3;
		double Iaccuracy = 1e-6;
		ret = getline_strip_comment(f, &line, &linesz);
		if (ret == -ERR_NO_DATA)
		{
			break;
		}
		if (ret == -ERR_NO_MEMORY)
		{
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		if (ret != 0)
		{
			fprintf(stderr, "Unexpected error\n");
			abort();
		}
		if (iswhiteonly(line))
		{
			continue;
		}
		//printf("Line: %s\n", line);
		lineptr = line;
		first = lineptr + nonspaceoff(lineptr);
		if (!*first)
		{
			fprintf(stderr, "Invalid line: %s\n", line);
		}
		sp = spaceoff(first);
		if (!first[sp])
		{
			fprintf(stderr, "Invalid line: %s\n", line);
		}
		first[sp] = '\0';
		lineptr = &first[sp+1];
		second = lineptr + nonspaceoff(lineptr);
		if (!*second)
		{
			fprintf(stderr, "Invalid line: %s\n", line);
		}
		sp = spaceoff(second);
		if (!second[sp])
		{
			fprintf(stderr, "Invalid line: %s\n", line);
		}
		second[sp] = '\0';
		lineptr = &second[sp+1];
		third = lineptr + nonspaceoff(lineptr);
		if (!*third)
		{
			fprintf(stderr, "Invalid line: %s\n", line);
		}
		sp = spaceoff(third);
		has_more = !!third[sp];
		third[sp] = '\0';
		lineptr = &third[sp+1];
		ln1 = strtol(first, &endptr, 10);
		if (ln1 < 0 || *first == '\0' || *endptr != '\0' || (long)(int)ln1 != ln1)
		{
			fprintf(stderr, "Not an int node: %ld\n", ln1);
			exit(1);
		}
		ln2 = strtol(second, &endptr, 10);
		if (ln2 < 0 || *first == '\0' || *endptr != '\0' || (long)(int)ln2 != ln2)
		{
			fprintf(stderr, "Not an int node: %ld\n", ln2);
			exit(1);
		}
		n1 = (int)ln1;
		n2 = (int)ln2;
		if (n1 == n2)
		{
			fprintf(stderr, "Two same nodes %d %d for %s\n", n1, n2, third);
			exit(1);
		}
		mark_node_seen(ctx, n1);
		mark_node_seen(ctx, n2);
		//printf("Mandatory tokens: %s %s %s\n", first, second, third);
		switch (*third)
		{
			case 'C':
				//printf("It's a capacitor\n");
				typ = TYPE_CAPACITOR;
				break;
			case 'L':
				//printf("It's an inductor\n");
				typ = TYPE_INDUCTOR;
				break;
			case 'V':
				//printf("It's a voltage source\n");
				typ = TYPE_VOLTAGE;
				break;
			case 'S':
				//printf("It's a switch\n");
				typ = TYPE_SWITCH;
				break;
			case 'D':
				//printf("It's a diode\n");
				typ = TYPE_DIODE;
				break;
			case 'd':
				//printf("It's a Shockley diode\n");
				typ = TYPE_SHOCKLEY_DIODE;
				break;
			case 'R':
				//printf("It's a resistor\n");
				typ = TYPE_RESISTOR;
				break;
			case 'T':
				//printf("It's a transformer\n");
				typ = TYPE_TRANSFORMER;
				break;
			case 'X':
				//printf("It's a transformer\n");
				typ = TYPE_TRANSFORMER_DIRECT;
				break;
			default:
				fprintf(stderr, "Can't determine what %s is\n", third);
				exit(1);
		}
		while (has_more)
		{
			char *more = lineptr + nonspaceoff(lineptr);
			char *equals;
			char *val;
			if (!*more)
			{
				break;
			}
			sp = spaceoff(more);
			if (!more[sp])
			{
				has_more = 0;
			}
			more[sp] = '\0';
			lineptr = &more[sp+1];
			//printf("Extra token: %s\n", more);
			equals = strchr(more, '=');
			if (equals == NULL)
			{
				fprintf(stderr, "Extra token no equals sign\n");
				exit(1);
			}
			*equals = '\0';
			val = &equals[1];
			if (strcmp(more, "R") == 0)
			{
				R = strtod(val, &endptr);
				if (R <= 0)
				{
					fprintf(stderr, "Invalid resistance: %lf\n", R);
					exit(1);
				}
			}
			else if (strcmp(more, "C") == 0)
			{
				if (typ != TYPE_CAPACITOR)
				{
					fprintf(stderr, "Only capacitors have capacitance\n");
					exit(1);
				}
				C = strtod(val, &endptr);
				if (C <= 0)
				{
					fprintf(stderr, "Invalid capacitance: %lf\n", C);
					exit(1);
				}
			}
			else if (strcmp(more, "L") == 0)
			{
				if (typ != TYPE_INDUCTOR)
				{
					fprintf(stderr, "Only inductors have inductance\n");
					exit(1);
				}
				L = strtod(val, &endptr);
				if (L <= 0)
				{
					fprintf(stderr, "Invalid inductance: %lf\n", L);
					exit(1);
				}
			}
			else if (strcmp(more, "V") == 0)
			{
				if (typ != TYPE_VOLTAGE)
				{
					fprintf(stderr, "Only voltage sources have voltage\n");
					exit(1);
				}
				V = strtod(val, &endptr);
				has_voltage = 1;
			}
			else if (strcmp(more, "Vinit") == 0)
			{
				if (typ != TYPE_CAPACITOR)
				{
					fprintf(stderr, "Only capacitors have initial voltage\n");
					exit(1);
				}
				Vinit = strtod(val, &endptr);
			}
			else if (strcmp(more, "Iinit") == 0)
			{
				if (typ != TYPE_INDUCTOR)
				{
					fprintf(stderr, "Only inductors have initial current\n");
					exit(1);
				}
				Iinit = strtod(val, &endptr);
			}
			else if (strcmp(more, "N") == 0)
			{
				if (typ != TYPE_TRANSFORMER && typ != TYPE_TRANSFORMER_DIRECT)
				{
					fprintf(stderr, "Only transformers have turns ratios\n");
					exit(1);
				}
				N = strtod(val, &endptr);
				if (N <= 0)
				{
					fprintf(stderr, "Invalid turns ratio: %lf\n", N);
					exit(1);
				}
			}
			else if (strcmp(more, "Lbase") == 0)
			{
				if (typ != TYPE_TRANSFORMER && typ != TYPE_TRANSFORMER_DIRECT)
				{
					fprintf(stderr, "Only transformers have base inductance\n");
					exit(1);
				}
				Lbase = strtod(val, &endptr);
				if (Lbase <= 0)
				{
					fprintf(stderr, "Invalid base inductance: %lf\n", Lbase);
					exit(1);
				}
			}
			else if (strcmp(more, "primary") == 0)
			{
				long lprimary;
				if (typ != TYPE_TRANSFORMER && typ != TYPE_TRANSFORMER_DIRECT)
				{
					fprintf(stderr, "Only transformers have primary and secondary windings\n");
					exit(1);
				}
				lprimary = strtol(val, &endptr, 10);
				if (lprimary != 0 && lprimary != 1)
				{
					fprintf(stderr, "Valid values for primary are 0 and 1\n");
					exit(1);
				}
				primary = !!lprimary;
			}
			else if (strcmp(more, "Vmin") == 0)
			{
				//if (typ != TYPE_TRANSFORMER)
				if (typ != TYPE_TRANSFORMER && typ != TYPE_TRANSFORMER_DIRECT)
				{
					fprintf(stderr, "Only transformers have minimum search voltage\n");
					exit(1);
				}
				Vmin = strtod(val, &endptr);
				has_vmin = 1;
			}
			else if (strcmp(more, "Vmax") == 0)
			{
				//if (typ != TYPE_TRANSFORMER)
				if (typ != TYPE_TRANSFORMER && typ != TYPE_TRANSFORMER_DIRECT && typ != TYPE_SHOCKLEY_DIODE)
				{
					fprintf(stderr, "Only transformers and Shockley diodes have maximum search voltage\n");
					exit(1);
				}
				Vmax = strtod(val, &endptr);
				has_vmax = 1;
			}
			else if (strcmp(more, "diode_threshold") == 0)
			{
				if (typ != TYPE_DIODE)
				{
					fprintf(stderr, "Only diodes have threshold\n");
					exit(1);
				}
				diode_threshold = strtod(val, &endptr);
				if (diode_threshold < 0)
				{
					fprintf(stderr, "Invalid diode threshold: %lf\n", diode_threshold);
					exit(1);
				}
			}
			else if (strcmp(more, "on_recalc") == 0)
			{
				long on_recalc_l;
				if (typ != TYPE_DIODE)
				{
					fprintf(stderr, "Only diodes have on_recalc\n");
					exit(1);
				}
				on_recalc_l = strtol(val, &endptr, 10);
				if (on_recalc_l != 0 && on_recalc_l != 1)
				{
					fprintf(stderr, "Invalid on_recalc: %d\n", on_recalc);
					exit(1);
				}
				on_recalc = on_recalc_l;
			}
			else if (strcmp(more, "VT") == 0)
			{
				if (typ != TYPE_SHOCKLEY_DIODE)
				{
					fprintf(stderr, "Only Shockley diodes have thermal voltage");
					exit(1);
				}
				VT = strtod(val, &endptr);
				if (VT <= 0)
				{
					fprintf(stderr, "Invalid thermal voltage: %lf\n", VT);
					exit(1);
				}
			}
			else if (strcmp(more, "Is") == 0)
			{
				if (typ != TYPE_SHOCKLEY_DIODE)
				{
					fprintf(stderr, "Only Shockley diodes have saturation current");
					exit(1);
				}
				Is = strtod(val, &endptr);
				if (Is <= 0)
				{
					fprintf(stderr, "Invalid saturation current: %lf\n", Is);
					exit(1);
				}
			}
			else if (strcmp(more, "Iaccuracy") == 0)
			{
				if (typ != TYPE_SHOCKLEY_DIODE)
				{
					fprintf(stderr, "Only Shockley diodes have current accuracy");
					exit(1);
				}
				Iaccuracy = strtod(val, &endptr);
				if (Iaccuracy <= 0)
				{
					fprintf(stderr, "Invalid current accuracy: %lf\n", Iaccuracy);
					exit(1);
				}
			}
			else
			{
				fprintf(stderr, "Invalid parameter: %s\n", more);
				exit(1);
			}
		}
		if (typ == TYPE_VOLTAGE && !has_voltage)
		{
			fprintf(stderr, "Voltage source %s must have voltage\n", third);
			exit(1);
		}
		if (typ == TYPE_TRANSFORMER && primary && !has_vmin)
		{
			fprintf(stderr, "Transformer primary %s must have minimum search voltage\n", third);
			exit(1);
		}
		if (typ == TYPE_TRANSFORMER && primary && !has_vmax)
		{
			fprintf(stderr, "Transformer primary %s must have maximum search voltage\n", third);
			exit(1);
		}
		if (typ == TYPE_SHOCKLEY_DIODE && !has_vmax)
		{
			Vmax = 1.5; // default value
		}
		if ((typ == TYPE_TRANSFORMER || typ == TYPE_TRANSFORMER_DIRECT) && primary && Lbase <= 0)
		{
			fprintf(stderr, "Transformer primary %s must have base inductance\n", third);
			exit(1);
		}
		if ((typ == TYPE_TRANSFORMER || typ == TYPE_TRANSFORMER_DIRECT) && !primary && has_vmin)
		{
			fprintf(stderr, "Transformer secondary %s must not have minimum search voltage\n", third);
			exit(1);
		}
		if ((typ == TYPE_TRANSFORMER || typ == TYPE_TRANSFORMER_DIRECT) && !primary && has_vmax)
		{
			fprintf(stderr, "Transformer secondary %s must not have maximum search voltage\n", third);
			exit(1);
		}
		if ((typ == TYPE_TRANSFORMER || typ == TYPE_TRANSFORMER_DIRECT) && !primary && Lbase > 0)
		{
			fprintf(stderr, "Transformer secondary %s must not have base inductance\n", third);
			exit(1);
		}
		if ((typ == TYPE_TRANSFORMER || typ == TYPE_TRANSFORMER_DIRECT) && N <= 0)
		{
			fprintf(stderr, "Transformer %s must have turns ratio\n", third);
			exit(1);
		}
		add_element_used(ctx, third, n1, n2, typ,
			V,
			Vinit,
			Iinit,
			L,
			R,
			C,
			N,
			Vmin,
			Vmax,
			Lbase,
			primary,
			diode_threshold,
			on_recalc,
			VT,
			Is,
			Iaccuracy);
	}
	fclose(f);
	free(line);
	linesz = 0;
}

void init_simulation(struct libsimul_ctx *ctx)
{
	check_dense_nodes(ctx);
	check_at_most_one_transformer(ctx);
	ctx->nodecnt = ctx->node_seen_sz - 1;
	ctx->G_matrix = malloc(sizeof(*ctx->G_matrix)*ctx->nodecnt*ctx->nodecnt);
	ctx->G_LU = malloc(sizeof(*ctx->G_LU)*ctx->nodecnt*ctx->nodecnt);
	ctx->G_ipiv = malloc(sizeof(*ctx->G_ipiv)*ctx->nodecnt);
	ctx->Isrc_vector = malloc(sizeof(*ctx->Isrc_vector)*ctx->nodecnt);
	ctx->V_vector = malloc(sizeof(*ctx->V_vector)*ctx->nodecnt);
	if (ctx->G_matrix == NULL || ctx->G_LU == NULL || ctx->G_ipiv == NULL ||
	    ctx->Isrc_vector == NULL || ctx->V_vector == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	form_g_matrix(ctx);
	calc_lu(ctx);
}

void recalc(struct libsimul_ctx *ctx)
{
	// If there is a Shockley diode, recalc will be done anyway
	if (!ctx->has_shockley)
	{
		form_g_matrix(ctx);
		calc_lu(ctx);
	}
}

void simulation_step(struct libsimul_ctx *ctx)
{
	size_t recalccnt = 0;
	int status;
	int recalc_loop = 0;
	if (ctx->has_shockley)
	{
		form_g_matrix(ctx);
		calc_lu(ctx);
	}
	form_isrc_vector(ctx);
	calc_V(ctx);
	while ((status = go_through_all(ctx, recalc_loop)) != 0)
	{
		//fprintf(stderr, "Recalc\n");
		recalccnt++;
		if (recalccnt == 1024)
		{
			//fprintf(stderr, "Recalc loop\n");
			recalc_loop = 1;
			break;
		}
		if (status != ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER || ctx->has_shockley)
		{
			form_g_matrix(ctx);
			calc_lu(ctx);
		}
		form_isrc_vector(ctx);
		calc_V(ctx);
	}
	if (recalc_loop)
	{
		recalccnt = 0;
		while ((status = go_through_all(ctx, recalc_loop)) != 0)
		{
			//fprintf(stderr, "Recalc\n");
			recalccnt++;
			if (recalccnt == 1024)
			{
				fprintf(stderr, "Recalc loop, can't handle\n");
				exit(1);
			}
			if (status != ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER || ctx->has_shockley)
			{
				form_g_matrix(ctx);
				calc_lu(ctx);
			}
			form_isrc_vector(ctx);
			calc_V(ctx);
		}
	}
	go_through_shockley_diodes_2(ctx);
}

void libsimul_init(struct libsimul_ctx *ctx, double dt)
{
	ctx->has_shockley = 0;
	ctx->dt = dt;
	ctx->elements_used = NULL;
	ctx->elements_used_sz = 0;
	ctx->elements_used_cap = 0;
	ctx->node_seen = NULL;
	ctx->node_seen_sz = 0;
	ctx->node_seen_cap = 0;
	ctx->xformerid = SIZE_MAX;
	ctx->xformerstate = STATE_FINI;
	ctx->Isrc_vector = NULL;
	ctx->V_vector = NULL;
	ctx->G_matrix = NULL;
	ctx->G_LU = NULL;
	ctx->G_ipiv = NULL;
}
void libsimul_free(struct libsimul_ctx *ctx)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		free(ctx->elements_used[i]->allptrs);
		free(ctx->elements_used[i]->name);
		free(ctx->elements_used[i]);
	}
	free(ctx->elements_used);
	free(ctx->node_seen);
	free(ctx->Isrc_vector);
	free(ctx->V_vector);
	free(ctx->G_matrix);
	free(ctx->G_LU);
	free(ctx->G_ipiv);
	libsimul_init(ctx, 0);
}
double get_resistor(struct libsimul_ctx *ctx, const char *rsname)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, rsname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Resistor %s not found\n", rsname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_RESISTOR)
	{
		fprintf(stderr, "Element %s not a resistor\n", rsname);
		exit(1);
	}
	return ctx->elements_used[i]->R;
}
double get_inductor(struct libsimul_ctx *ctx, const char *indname)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, indname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Inductor %s not found\n", indname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_INDUCTOR)
	{
		fprintf(stderr, "Element %s not a inductor\n", indname);
		exit(1);
	}
	return ctx->elements_used[i]->L;
}
double get_capacitor(struct libsimul_ctx *ctx, const char *capname)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, capname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Capacitor %s not found\n", capname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_CAPACITOR)
	{
		fprintf(stderr, "Element %s not a capacitor\n", capname);
		exit(1);
	}
	return ctx->elements_used[i]->C;
}
double get_voltage_source_current(struct libsimul_ctx *ctx, const char *vsname)
{
	size_t i;
	double V1;
	double V2;
	double R;
	double V;
	double V_ext;
	double V_diff;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, vsname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Voltage source %s not found\n", vsname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_VOLTAGE)
	{
		fprintf(stderr, "Element %s not a voltage source\n", vsname);
		exit(1);
	}
	V1 = get_V(ctx, ctx->elements_used[i]->n1);
	V2 = get_V(ctx, ctx->elements_used[i]->n2);
	R = ctx->elements_used[i]->R;
	V = ctx->elements_used[i]->V;
	V_ext = V1 - V2;
	V_diff = V - V_ext;
	//printf("V1 %g V2 %g R %g V %g V_ext %g V_diff %g I %g\n", V1, V2, R, V, V_ext, V_diff, V_diff/R);
	return V_diff/R;
}
void set_capacitor_voltage(struct libsimul_ctx *ctx, const char *capname, double V)
{
	size_t i;
	for (i = 0; i < ctx->elements_used_sz; i++)
	{
		if (strcmp(ctx->elements_used[i]->name, capname) == 0)
		{
			break;
		}
	}
	if (i == ctx->elements_used_sz)
	{
		fprintf(stderr, "Capacitor %s not found\n", capname);
		exit(1);
	}
	if (ctx->elements_used[i]->typ != TYPE_CAPACITOR)
	{
		fprintf(stderr, "Element %s not a capacitor\n", capname);
		exit(1);
	}
	ctx->elements_used[i]->I_src = V/ctx->elements_used[i]->R;
}
