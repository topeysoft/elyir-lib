void elyir_begin(void);
bool mgos_elyir_lib_init(void);

void save_state(char *state);
const char *get_state();

// static void process_state(bool on);
// static void restore_def_state();
// void set_state_handler(struct state_handler);