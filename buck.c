#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lapack.h>
#include <math.h>

const double dt = 1e-7; // 100 ns

enum {
	ERR_NO_ERROR = 0,
	ERR_HAVE_TO_SIMULATE_AGAIN = 1,
	ERR_NO_MEMORY = 2,
	ERR_NO_DATA = 3,
};

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

enum element_type {
	TYPE_RESISTOR,
	TYPE_CAPACITOR,
	TYPE_INDUCTOR,
	TYPE_SWITCH,
	TYPE_VOLTAGE,
	TYPE_DIODE,
};

double *Isrc_vector;
double *V_vector;
double *G_matrix;
double *G_LU;
int *G_ipiv;
size_t nodecnt;

// Convention: I_src is facing from n2 to n1
// Convention: V is V_n_1 - V_n_2
// Then positive V is positive I_src
// Diode convention: arrow pointing from n1 (anode) to n2 (cathode)
struct element {
	char *name;
	int n1;
	int n2;
	int current_switch_state_is_closed;
	enum element_type typ;
	double V;
	double I_src;
	double Vinit;
	double Iinit;
	double L;
	double R;
	double C;
};

struct element *elements_used = NULL;
size_t elements_used_sz = 0;
size_t elements_used_cap = 0;

unsigned char *node_seen = NULL;
size_t node_seen_sz = 0;
size_t node_seen_cap = 0;

int set_switch_state(const char *swname, int state)
{
	size_t i;
	for (i = 0; i < elements_used_sz; i++)
	{
		if (strcmp(elements_used[i].name, swname) == 0)
		{
			break;
		}
	}
	if (i == elements_used_sz)
	{
		fprintf(stderr, "Switch %s not found\n", swname);
		exit(1);
	}
	if (elements_used[i].typ != TYPE_SWITCH)
	{
		fprintf(stderr, "Element %s not a switch\n", swname);
		exit(1);
	}
	if ((!!elements_used[i].current_switch_state_is_closed) == (!!state))
	{
		return 0;
	}
	elements_used[i].current_switch_state_is_closed = !!state;
	for (i = 0; i < elements_used_sz; i++)
	{
		if (elements_used[i].typ == TYPE_DIODE)
		{
			// FIXME spooky...
			elements_used[i].current_switch_state_is_closed = 1;
		}
	}
	return ERR_HAVE_TO_SIMULATE_AGAIN;
}

void mark_node_seen(int n)
{
	size_t nsz;
	size_t i;
	if (n < 0)
	{
		abort();
	}
	nsz = (size_t)n;
	if (node_seen == NULL || nsz >= node_seen_cap)
	{
		size_t newcap = nsz*2 + 16;
		unsigned char *new_ns;
		new_ns = realloc(node_seen, sizeof(*node_seen)*newcap);
		if (new_ns == NULL)
		{
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		node_seen = new_ns;
		node_seen_cap = newcap;
		for (i = node_seen_sz; i < newcap; i++)
		{
			new_ns[i] = 0;
		}
	}
	node_seen[nsz] = 1;
	if (nsz+1 > node_seen_sz)
	{
		node_seen_sz = nsz+1;
	}
}
void check_dense_nodes(void)
{
	size_t i;
	if (node_seen_sz < 2)
	{
		fprintf(stderr, "Must have at least 2 nodes\n");
		exit(1);
	}
	for (i = 0; i < node_seen_sz; i++)
	{
		if (!node_seen[i])
		{
			fprintf(stderr, "Node %zu not seen\n", i);
			exit(1);
		}
	}
}

void form_g_matrix(void)
{
	struct element *el;
	size_t x,y;
	size_t i;
	for (x = 0; x < nodecnt; x++)
	{
		for (y = 0; y < nodecnt; y++)
		{
			// Column major
			G_matrix[x*nodecnt+y] = 0;
		}
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		int n1;
		int n2;
		double G;
		el = &elements_used[i];
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
			G_matrix[(n1-1)*nodecnt+(n1-1)] += G;
		}
		if (n2 != 0)
		{
			G_matrix[(n2-1)*nodecnt+(n2-1)] += G;
		}
		if (n1 != 0 && n2 != 0)
		{
			G_matrix[(n1-1)*nodecnt+(n2-1)] -= G;
			G_matrix[(n2-1)*nodecnt+(n1-1)] -= G;
		}
	}
}
void calc_lu(void)
{
	const int n = nodecnt;
	int info = 0;
	memcpy(G_LU, G_matrix, sizeof(*G_LU)*nodecnt*nodecnt);
	LAPACK_dgetrf(&n, &n, G_LU, &n, G_ipiv, &info);
	if (info != 0)
	{
		fprintf(stderr, "Can't LU decompose: %d\n", info);
		exit(1);
	}
}
void calc_V(void)
{
	const int n = nodecnt;
	const int one = 1;
	int info = 0;
	memcpy(V_vector, Isrc_vector, sizeof(*V_vector)*nodecnt);
	LAPACK_dgetrs("N", &n, &one, G_LU, &n, G_ipiv, V_vector, &n, &info);
	if (info != 0)
	{
		fprintf(stderr, "Can't solve system of equations\n");
		exit(1);
	}
}
void form_isrc_vector(void)
{
	struct element *el;
	size_t i;
	size_t x;
	for (x = 0; x < nodecnt; x++)
	{
		Isrc_vector[x] = 0;
	}
	for (i = 0; i < elements_used_sz; i++)
	{
		int n1;
		int n2;
		double Isrc;
		el = &elements_used[i];
		n1 = el->n1;
		n2 = el->n2;
		Isrc = el->I_src;
		if (n1 != 0)
		{
			Isrc_vector[n1-1] += Isrc;
		}
		if (n2 != 0)
		{
			Isrc_vector[n2-1] -= Isrc;
		}
	}
}

double get_V(int node)
{
	if (node == 0)
	{
		return 0;
	}
	return V_vector[node-1];
}

// Return: 0 OK
// Return: -EAGAIN have to do simulation again
int go_through_diodes(void)
{
	size_t i;
	int ret = 0;
	double V_across_diode;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = &elements_used[i];
		if (el->typ != TYPE_DIODE)
		{
			continue;
		}
		V_across_diode = get_V(el->n1) - get_V(el->n2);
		if (V_across_diode < 0 && el->current_switch_state_is_closed)
		{
			el->current_switch_state_is_closed = 0;
			ret = ERR_HAVE_TO_SIMULATE_AGAIN;
		}
		if (V_across_diode > 0 && !el->current_switch_state_is_closed)
		{
			el->current_switch_state_is_closed = 1;
			ret = ERR_HAVE_TO_SIMULATE_AGAIN;
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
void go_through_inductors(void)
{
	size_t i;
	double V_across_inductor;
	double dI;
	int oldsign, newsign;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = &elements_used[i];
		if (el->typ != TYPE_INDUCTOR)
		{
			continue;
		}
		V_across_inductor = get_V(el->n1) - get_V(el->n2);
		dI = -V_across_inductor/el->L*dt;
		if (fabs(V_across_inductor) > 100)
		{
			fprintf(stderr, "Limit1 reached\n");
			exit(1);
		}
		oldsign = signum(el->I_src);
		el->I_src += dI;
		newsign = signum(el->I_src);
		if (oldsign != 0 && newsign != 0 && oldsign != newsign)
		{
			// Probably wisest to reset to zero
			el->I_src = 0;
		}
		if (fabs(el->I_src) > 100)
		{
			fprintf(stderr, "Limit2 reached\n");
			exit(1);
		}
		//fprintf(stderr, "Inductor V_across %g I_src %g\n", V_across_inductor, el->I_src);
	}
}
void go_through_capacitors(void)
{
	size_t i;
	double V_across_capacitor;
	double I_R;
	double I_tot;
	double dU;
	for (i = 0; i < elements_used_sz; i++)
	{
		struct element *el = &elements_used[i];
		if (el->typ != TYPE_CAPACITOR)
		{
			continue;
		}
		V_across_capacitor = get_V(el->n1) - get_V(el->n2);
		I_R = V_across_capacitor/el->R;
		I_tot = el->I_src - I_R;
		// Positive I_tot: discharge
		// Negative I_tot: charge
		dU = -I_tot/el->C*dt;
		el->I_src += dU/el->R;
	}
}
int go_through_all(void)
{
	int ret = 0;
	ret = go_through_diodes();
	if (ret != 0)
	{
		return ret;
	}
	go_through_inductors();
	go_through_capacitors();
	return 0;
}

int add_element_used(const char *element, int n1, int n2, enum element_type typ,
	double V,
	double Vinit,
	double Iinit,
	double L,
	double R,
	double C)
{
	size_t i;

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

	for (i = 0; i < elements_used_sz; i++)
	{
		if (strcmp(elements_used[i].name, element) == 0)
		{
			fprintf(stderr, "Element %s already used\n", element);
			exit(1);
		}
	}
	if (elements_used_sz >= elements_used_cap || elements_used == NULL)
	{
		struct element *new_eu;
		size_t new_cap = 2*elements_used_sz+16;
		new_eu = realloc(elements_used, sizeof(*elements_used)*new_cap);
		if (new_eu == NULL)
		{
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		elements_used = new_eu;
		elements_used_cap = new_cap;
	}
	elements_used[elements_used_sz].name = strdup(element);
	elements_used[elements_used_sz].n1 = n1;
	elements_used[elements_used_sz].n2 = n2;
	elements_used[elements_used_sz].typ = typ;
	elements_used[elements_used_sz].V = V;
	elements_used[elements_used_sz].Vinit = Vinit;
	elements_used[elements_used_sz].Iinit = Iinit;
	elements_used[elements_used_sz].L = L;
	elements_used[elements_used_sz].R = R;
	elements_used[elements_used_sz].C = C;
	elements_used[elements_used_sz].I_src = 0;
	elements_used[elements_used_sz].current_switch_state_is_closed = 1;
	if (typ == TYPE_VOLTAGE)
	{
		elements_used[elements_used_sz].I_src = V/R;
	}
	if (typ == TYPE_CAPACITOR)
	{
		elements_used[elements_used_sz].I_src = Vinit/R;
	}
	if (typ == TYPE_INDUCTOR)
	{
		elements_used[elements_used_sz].I_src = Iinit;
	}
	elements_used_sz++;
	return 0;
}

int main(int argc, char **argv)
{
	char *line = NULL;
	size_t linesz = 0;
	size_t i;
	FILE *f;
	f = fopen("buck.txt", "r");
	if (f == NULL)
	{
		fprintf(stderr, "Can't open file\n");
		return 1;
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
		int has_voltage;
		double R = 0;
		double V = 0;
		double L = 0;
		double C = 0;
		double Vinit = 0;
		double Iinit = 0;
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
		mark_node_seen(n1);
		mark_node_seen(n2);
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
					fprintf(stderr, "Invalid capacitance: %lf\n", R);
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
					fprintf(stderr, "Invalid inductance: %lf\n", R);
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
		add_element_used(third, n1, n2, typ,
			V,
			Vinit,
			Iinit,
			L,
			R,
			C);
	}
	check_dense_nodes();
	nodecnt = node_seen_sz - 1;
	G_matrix = malloc(sizeof(*G_matrix)*nodecnt*nodecnt);
	G_LU = malloc(sizeof(*G_LU)*nodecnt*nodecnt);
	G_ipiv = malloc(sizeof(*G_ipiv)*nodecnt);
	Isrc_vector = malloc(sizeof(*Isrc_vector)*nodecnt);
	V_vector = malloc(sizeof(*V_vector)*nodecnt);
	if (G_matrix == NULL || G_LU == NULL || G_ipiv == NULL ||
	    Isrc_vector == NULL || V_vector == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	form_g_matrix();
	calc_lu();
	int switch_state = 1;
	int cnt_remain = 500;
	set_switch_state("S1", switch_state);
	//for (i = 0; i < 449800; i++)
	for (i = 0; i < 5*1000*1000; i++)
	{
		form_isrc_vector();
		calc_V();
		while (go_through_all() != 0)
		{
			fprintf(stderr, "Recalc\n");
			form_g_matrix();
			calc_lu();
			form_isrc_vector();
			calc_V();
		}
		printf("%zu %g\n", i, get_V(4));
		if (get_V(4) > 13.21*100)
		{
			break;
		}
		cnt_remain--;
		if (cnt_remain == 0)
		{
			switch_state = !switch_state;
			//switch_state = 1;
			if (set_switch_state("S1", switch_state) != 0)
			{
				form_g_matrix();
				calc_lu();
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
	free(line);
	linesz = 0;
	return 0;
}
