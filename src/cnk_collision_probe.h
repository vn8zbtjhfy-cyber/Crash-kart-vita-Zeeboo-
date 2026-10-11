#ifndef CNK_COLLISION_PROBE_H
#define CNK_COLLISION_PROBE_H
/* Optional developer-only collision experiments, never Zeebo defaults.
 * Exact TSV format: WALLS\t0|1 and KARTS\t0|1, once each.
 * Missing file returns -2, corrupt file leaves output unchanged. */
typedef struct { int walls; int karts; } cnk_collision_probe;
int cnk_collision_probe_read(const char *path,cnk_collision_probe *out);
#endif
