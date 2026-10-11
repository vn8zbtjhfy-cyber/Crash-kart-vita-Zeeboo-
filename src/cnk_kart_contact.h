#ifndef CNK_KART_CONTACT_H
#define CNK_KART_CONTACT_H
#include "cnk_drive.h"
#include <stdint.h>
/* EXPERIMENTAL two-dimensional kart body approximation. Not extracted from
 * Zeebo. Racing routes may pass over/under each other: height is checked too. */
#define CNK_KART_CONTACT_MAX 6

typedef struct {
 float radius;          /* provisional horizontal collision-disc radius */
 float height_tolerance;/* skip karts on different vertical levels */
} cnk_kart_contact_tuning;
void cnk_kart_contact_default(cnk_kart_contact_tuning *out);
int cnk_kart_contact_validate(const cnk_kart_contact_tuning *cfg);
/* Input `before[i]` and `karts[i]` are old and predicted states for one frame.
 * movable[i]==0 fixes that participant in place (for finished racers).
 * Performs swept overlap tests followed by positional separation. Collision
 * flags and velocities are kept separate from CNK_RACE_* gate events.
 * Returns number of distinct pairs that made contact, <0 on invalid inputs;
 * invalid input leaves all kart states and reported flags untouched. */
int cnk_kart_contacts_resolve(cnk_kart *karts,const cnk_vec3 *before,
  const uint8_t *movable,uint32_t count,
  const cnk_kart_contact_tuning *cfg,uint8_t *contact_flags);
#endif
