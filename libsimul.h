#ifndef _LIBSIMUL_H_
#define _LIBSIMUL_H_

const double dt;

enum {
	ERR_NO_ERROR = 0,
	ERR_HAVE_TO_SIMULATE_AGAIN = 1,
	ERR_NO_MEMORY = 2,
	ERR_NO_DATA = 3,
};

int iswhiteonly(const char *ln);
size_t spaceoff(const char *ln);
size_t nonspaceoff(const char *ln);
int getline_strip_comment(FILE *f, char **ln, size_t *lnsz);

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

struct element *elements_used;
size_t elements_used_sz;
size_t elements_used_cap;

unsigned char *node_seen;
size_t node_seen_sz;
size_t node_seen_cap;

int set_switch_state(const char *swname, int state);
void mark_node_seen(int n);
void check_dense_nodes(void);
void form_g_matrix(void);
void calc_lu(void);
void calc_V(void);
void form_isrc_vector(void);
double get_V(int node);
int go_through_diodes(void);
int signum(double d);
void go_through_inductors(void);
void go_through_capacitors(void);
int go_through_all(void);
int add_element_used(const char *element, int n1, int n2, enum element_type typ,
	double V,
	double Vinit,
	double Iinit,
	double L,
	double R,
	double C);
void read_file(const char *fname);
void init_simulation(void);
void recalc(void);
void simulation_step(void);

#endif
