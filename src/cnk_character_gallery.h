#ifndef CNK_CHARACTER_GALLERY_H
#define CNK_CHARACTER_GALLERY_H
#include "cnk_verified_characters.h"
#include "cnk_camera.h"
/* Development-only preview of three original Zeebo 3DOB character models.
 * Neither a playable character selection nor a driver/kart assignment.
 * Source model bounds determine the VIEWER camera only. No source vertices
 * or character animation are modified. GPU calls require a VitaGL context. */
int cnk_character_gallery_camera_at(const cnk_object_mesh *mesh,float angle,cnk_camera *out);
int cnk_character_gallery_camera(const cnk_object_mesh *mesh,cnk_camera *out);
int cnk_character_gallery_draw_at(const cnk_character_pack *pack,float angle,unsigned *calls);
/* Returns 0 and total GPU draw calls on success. Validation is performed
 * for all three meshes before touching GPU state. Restores full viewport
 * on both successful and failed draws. */
int cnk_character_gallery_draw(const cnk_character_pack *pack,unsigned *calls);
#endif
