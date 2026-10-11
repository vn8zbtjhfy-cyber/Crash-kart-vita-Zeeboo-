#ifndef CNK_DRIVER_BINDINGS_H
#define CNK_DRIVER_BINDINGS_H
#include "cnk_verified_characters.h"
#include "cnk_racer_visual.h"
#include "cnk_session.h"
#include <stdint.h>
/* Driver seats are NOT inferred from game resource order. This OPTIONAL,
 * independently reviewed sidecar says which verified character is placed
 * relative to the kart pose. Units/seat orientation must be user-calibrated.
 * TSV, exactly 18 fields: racer slot (0..2), crash|coco|ripper_roo,
 * 16 column-major affine local_from_mesh floats. No guesses ship here. */
typedef struct {
 uint8_t assigned[CNK_MAX_RACERS];
 cnk_character_identity identity[CNK_MAX_RACERS];
 float calibration[CNK_MAX_RACERS][16];
 unsigned count;
} cnk_driver_bindings;
/* Returns -2 for nonexistent optional file, other negative for corruption;
 * failed parse leaves *out zeroed. Strict finite invertible affine matrices. */
int cnk_driver_bindings_load(const char *path,cnk_driver_bindings *out);
/* Preflight verifies all slots have paired kart model assignments and all
 * verified characters are loaded BEFORE setting a single attachment. */
int cnk_driver_bindings_attachments(const cnk_driver_bindings *bindings,
        const cnk_character_pack *pack,const uint8_t kart_slots[CNK_MAX_RACERS],
        cnk_racer_visual_attachment out[CNK_MAX_RACERS]);
#endif
