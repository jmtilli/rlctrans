#ifndef _LIBSIMUL_H_
#define _LIBSIMUL_H_

enum {
	ERR_NO_ERROR = 0,
	ERR_HAVE_TO_SIMULATE_AGAIN = 1,
	ERR_HAVE_TO_SIMULATE_AGAIN_DIODE = 2,
	ERR_HAVE_TO_SIMULATE_AGAIN_TRANSFORMER = 3,
	ERR_HAVE_TO_SIMULATE_AGAIN_SHOCKLEY_DIODE = 4,
	ERR_NO_MEMORY = 5,
	ERR_NO_DATA = 6,
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
	TYPE_SHOCKLEY_DIODE,
	TYPE_TRANSFORMER,
	TYPE_TRANSFORMER_DIRECT,
};

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
	double V_across_diode;
	double V;
	double I_src;
	double I_model;
	double Vinit;
	double Iinit;
	double L;
	double R;
	double G_shockley;
	double G_R_shockley;
	double C;
	double N;
	double Vmin;
	double Vmax;
	double Lbase;
	double I_s; // Shockley diode saturation current
	double V_T; // Shockley diode thermal voltage
	double I_accuracy; // Shockley diode current accuracy
	double cur_phi_single; // only for primary
	double dphi_single; // only for primary
	int primary;
	struct element *primaryptr;
	struct element **allptrs;
	size_t allptrs_size;
	size_t allptrs_capacity;
	double transformer_direct_denom;
	double transformer_direct_const;
	double diode_threshold;
	int on_recalc;
	double expval;
};

enum xformerstatetype {
        STATE_LOBO,
        STATE_LOBOPOST,
        STATE_HIBO,
        STATE_HIBOPOST,
        STATE_ITER,
        STATE_ITERPOST,
        STATE_FINI,
};

struct libsimul_ctx {
	int has_shockley;
	double dt;
	double diode_threshold;

	double *Isrc_vector;
	double *V_vector;
	double *G_matrix;
	double *G_LU;
	int *G_ipiv;
	size_t nodecnt;

	struct element **elements_used;
	size_t elements_used_sz;
	size_t elements_used_cap;

	unsigned char *node_seen;
	size_t node_seen_sz;
	size_t node_seen_cap;

	size_t xformerid;
	enum xformerstatetype xformerstate;
	double loboV;
	double hiboV;
	double trialV;
	double lobophi;
	double hibophi;
	double trialphi;
};

void libsimul_init(struct libsimul_ctx *ctx, double dt);
void libsimul_free(struct libsimul_ctx *ctx);

double get_voltage_source_current(struct libsimul_ctx *ctx, const char *vsname);
void set_voltage_source(struct libsimul_ctx *ctx, const char *vsname, double V);
void set_capacitor_voltage(struct libsimul_ctx *ctx, const char *capname, double V);
int set_resistor(struct libsimul_ctx *ctx, const char *rsname, double R);
int set_inductor(struct libsimul_ctx *ctx, const char *indname, double L);
double get_resistor(struct libsimul_ctx *ctx, const char *rsname);
double get_inductor(struct libsimul_ctx *ctx, const char *indname);
double get_capacitor(struct libsimul_ctx *ctx, const char *capname);
double get_inductor_current(struct libsimul_ctx *ctx, const char *indname);
double get_transformer_inductor(struct libsimul_ctx *ctx, const char *xfrname);
double get_transformer_mag_current(struct libsimul_ctx *ctx, const char *xfrname);
int set_switch_state(struct libsimul_ctx *ctx, const char *swname, int state);
void mark_node_seen(struct libsimul_ctx *ctx, int n);
void check_dense_nodes(struct libsimul_ctx *ctx);
void form_g_matrix(struct libsimul_ctx *ctx);
void calc_lu(struct libsimul_ctx *ctx);
void calc_V(struct libsimul_ctx *ctx);
void form_isrc_vector(struct libsimul_ctx *ctx);
double get_V(struct libsimul_ctx *ctx, int node);
int go_through_diodes(struct libsimul_ctx *ctx, int recalc_loop);
int signum(double d);
void go_through_inductors(struct libsimul_ctx *ctx);
void go_through_capacitors(struct libsimul_ctx *ctx);
int go_through_all(struct libsimul_ctx *ctx, int recalc_loop);
int add_element_used(
	struct libsimul_ctx *ctx,
	const char *element, int n1, int n2, enum element_type typ,
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
	double Iaccuracy);
void read_file(struct libsimul_ctx *ctx, const char *fname);
void init_simulation(struct libsimul_ctx *ctx);
void recalc(struct libsimul_ctx *ctx);
void simulation_step(struct libsimul_ctx *ctx);
void check_at_most_one_transformer(struct libsimul_ctx *ctx);
int set_diode_hint(struct libsimul_ctx *ctx, const char *dname, int state);

#endif
