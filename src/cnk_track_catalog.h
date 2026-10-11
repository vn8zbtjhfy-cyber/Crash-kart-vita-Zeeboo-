#ifndef CNK_TRACK_CATALOG_H
#define CNK_TRACK_CATALOG_H

#include "fufs.h"
#include "cnk_scene.h"
#include <stdint.h>
#include <stddef.h>

/* Source-backed scene selection for the eventual native game. No invented
 * track names, ordering, characters or gameplay modes. 'RACE' means our
 * CURRENT reconstruction can validate DRV0/LAP and STRT, NOT that a Zeebo
 * game mode or menu entry has been reverse engineered. */
enum cnk_track_kind {
 CNK_TRACK_RACE=1,
 CNK_TRACK_OTHER=2
};
typedef struct {
 uint32_t vfs_index, resource_id;
 enum cnk_track_kind kind;
 uint32_t drive_points, gates;
 float route_length;
 cnk_vec3 original_start;
 float original_heading;
 int has_original_start;
} cnk_track_entry;
#define CNK_MAX_SOURCE_SCENES 64u
typedef struct {
 const fufs_file *source; /* borrowed, must remain alive */
 cnk_track_entry entries[CNK_MAX_SOURCE_SCENES];
 uint32_t count, race_count;
} cnk_track_catalog;

/* Build from PIFF/SCEN resources in the *user-provided* VFS. Atomic on
 * parse errors: out is left zeroed. No guessing based on VFS indices.
 * Return negative on corrupt scene, invalid resource or capacity overflow. */
int cnk_track_catalog_build(const fufs_file *vfs,cnk_track_catalog *out);
/* Entries appear in the original FUFS table order. race_number is zero-based
 * among ONLY verified RACE scenes; return NULL if not present. */
const cnk_track_entry *cnk_track_race_at(const cnk_track_catalog *catalog,
                                         uint32_t race_number);
const cnk_track_entry *cnk_track_find(const cnk_track_catalog *catalog,
                                     uint32_t vfs_index);
/* Native game entrance, integrated with cnk_game's source STRT launch. */
struct cnk_game;
#endif
