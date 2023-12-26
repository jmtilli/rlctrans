#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lapack.h>
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
			ctx->elements_used[i]->current_switch_state_is_closed = 1;
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
		n1 = el->n1;
		n2 = el->n2;
		Isrc = el->I_src;
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

double get_V(struct libsimul_ctx *ctx, int node)
{
	if (node == 0)
	{
		return 0;
	}
	return ctx->V_vector[node-1];
}

// Return: 0 OK
// Return: -EAGAIN have to do simulation again
int go_through_diodes(struct libsimul_ctx *ctx)
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
		V_across_diode = get_V(ctx, el->n1) - get_V(ctx, el->n2);
		if (V_across_diode < -ctx->diode_threshold && el->current_switch_state_is_closed)
		{
			el->current_switch_state_is_closed = 0;
			ret = ERR_HAVE_TO_SIMULATE_AGAIN_DIODE;
		}
		if (V_across_diode > ctx->diode_threshold && !el->current_switch_state_is_closed)
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

void set_transformer_voltage(struct libsimul_ctx *ctx, size_t el_id, double V)
{
	size_t i;
	ctx->elements_used[el_id]->I_src =
		V /
		ctx->elements_used[el_id]->R;
#if 0
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
	for (i = 0; i < ctx->elements_used[el_id]->secondaryptrs_size; i++)
	{
		struct element *el = ctx->elements_used[el_id]->secondaryptrs[i];
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
	V_across_winding =
		get_V(ctx, ctx->elements_used[el_id]->n1) -
		get_V(ctx, ctx->elements_used[el_id]->n2);
	I_R = V_across_winding/ctx->elements_used[el_id]->R;
	I_tot = ctx->elements_used[el_id]->I_src - I_R;
	phi_single -=
		I_tot * 
		ctx->elements_used[el_id]->Lbase *
		ctx->elements_used[el_id]->N;
#if 0
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
	for (i = 0; i < ctx->elements_used[el_id]->secondaryptrs_size; i++)
	{
		struct element *el = ctx->elements_used[el_id]->secondaryptrs[i];
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

int go_through_all(struct libsimul_ctx *ctx)
{
	int ret = 0;
	ret = go_through_diodes(ctx);
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
	int primary)
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
	if (typ != TYPE_INDUCTOR && R <= 0)
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
	el->secondaryptrs = NULL;
	el->secondaryptrs_size = 0;
	el->secondaryptrs_capacity = 0;
	el->cur_phi_single = 0;
	el->dphi_single = 0;
	el->current_switch_state_is_closed = 1;
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
					if (ctx->elements_used[j]->secondaryptrs == NULL || ctx->elements_used[j]->secondaryptrs_size >= ctx->elements_used[j]->secondaryptrs_capacity)
					{
						struct element **sec2;
						size_t new_cap = ctx->elements_used[j]->secondaryptrs_size*2+16;
						sec2 = realloc(ctx->elements_used[j]->secondaryptrs, sizeof(*sec2)*new_cap);
						if (sec2 == NULL)
						{
							fprintf(stderr, "Out of memory\n");
							exit(1);
						}
						ctx->elements_used[j]->secondaryptrs = sec2;
					}
					ctx->elements_used[j]->secondaryptrs[ctx->elements_used[j]->secondaryptrs_size++] = ctx->elements_used[i];
				}
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
		double Lbase = 0;
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
			case 'R':
				//printf("It's a resistor\n");
				typ = TYPE_RESISTOR;
				break;
			case 'T':
				//printf("It's a transformer\n");
				typ = TYPE_TRANSFORMER;
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
				if (typ == TYPE_INDUCTOR)
				{
					fprintf(stderr, "Inductor resistance should be a separate element\n");
					exit(1);
				}
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
				if (typ != TYPE_TRANSFORMER)
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
				if (typ != TYPE_TRANSFORMER)
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
				if (typ != TYPE_TRANSFORMER)
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
				if (typ != TYPE_TRANSFORMER)
				{
					fprintf(stderr, "Only transformers have minimum search voltage\n");
					exit(1);
				}
				Vmin = strtod(val, &endptr);
				has_vmin = 1;
			}
			else if (strcmp(more, "Vmax") == 0)
			{
				if (typ != TYPE_TRANSFORMER)
				{
					fprintf(stderr, "Only transformers have maximum search voltage\n");
					exit(1);
				}
				Vmax = strtod(val, &endptr);
				has_vmax = 1;
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
		if (typ == TYPE_TRANSFORMER && primary && Lbase <= 0)
		{
			fprintf(stderr, "Transformer primary %s must have base inductance\n", third);
			exit(1);
		}
		if (typ == TYPE_TRANSFORMER && !primary && has_vmin)
		{
			fprintf(stderr, "Transformer secondary %s must not have minimum search voltage\n", third);
			exit(1);
		}
		if (typ == TYPE_TRANSFORMER && !primary && has_vmax)
		{
			fprintf(stderr, "Transformer secondary %s must not have maximum search voltage\n", third);
			exit(1);
		}
		if (typ == TYPE_TRANSFORMER && !primary && Lbase > 0)
		{
			fprintf(stderr, "Transformer secondary %s must not have base inductance\n", third);
			exit(1);
		}
		if (typ == TYPE_TRANSFORMER && N <= 0)
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
			primary);
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
	form_g_matrix(ctx);
	calc_lu(ctx);
}

void simulation_step(struct libsimul_ctx *ctx)
{
	size_t recalccnt = 0;
	int status;
	form_isrc_vector(ctx);
	calc_V(ctx);
	while ((status = go_through_all(ctx)) != 0)
	{
		//fprintf(stderr, "Recalc\n");
		recalccnt++;
		if (recalccnt == 1024)
		{
			fprintf(stderr, "Recalc loop\n");
			exit(1);
		}
		if (status != ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER)
		{
			form_g_matrix(ctx);
			calc_lu(ctx);
		}
		form_isrc_vector(ctx);
		calc_V(ctx);
	}
}

void libsimul_init(struct libsimul_ctx *ctx, double dt, double diode_threshold)
{
	ctx->dt = dt;
	ctx->diode_threshold = diode_threshold;
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
		free(ctx->elements_used[i]->secondaryptrs);
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
	libsimul_init(ctx, 0, 0);
}
