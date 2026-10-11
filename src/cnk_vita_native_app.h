#ifndef CNK_VITA_NATIVE_APP_H
#define CNK_VITA_NATIVE_APP_H
/* A native vitaGL window/input loop, not a finished game or VPK.
 * The caller must package the exact user-owned data.vfs at vfs_path.
 * Returns negative on initialization/runtime error, 0 after requested exit.
 * This is a callable entry point; no installable is generated. */
int cnk_vita_native_run(const char *vfs_path);
/* Optional external file containing independently verified texture IDs.
 * Missing file leaves scenes untextured; invalid/mismatched mapping fails.
 * This never infers material associations and never generates a VPK. */
int cnk_vita_native_run_with_map(const char *vfs_path,const char *manifest_path);
/* Optional separate sidecar of explicitly assigned FUFS 3DOB resources and
 * 4x4 affine model calibration. No fallback models or inferred kart identity. */
int cnk_vita_native_run_with_assets(const char *vfs_path,
     const char *manifest_path,const char *racer_map_path);
/* Optional reviewed source sound-cue map, played on a dedicated Vita
 * audio thread. Absence is silent (never guesses effects). */
int cnk_vita_native_run_with_resources(const char *vfs_path,
    const char *manifest_path,const char *racer_map_path,const char *sound_map_path);
/* Optional *independently verified* music mapping. Transitions load
 * original decoded WAV tracks from music_directory, without guessing
 * which MP3 was used in any menu or circuit. */
int cnk_vita_native_run_with_music(const char *vfs_path,
    const char *manifest_path,const char *racer_map_path,
    const char *sound_map_path,const char *music_map_path,
    const char *music_directory);
/* Optional separately reviewed driver-in-kart assignments: each slot must
 * already have a verified kart model. No guessed driver/vehicle pairings. */
int cnk_vita_native_run_with_driver_assets(const char *vfs_path,
    const char *manifest_path,const char *racer_map_path,
    const char *sound_map_path,const char *music_map_path,
    const char *music_directory,const char *driver_map_path);
/* Optional independently validated kart texture for each original 3DOB
 * main-body TEXL name. Same PNG is reused across all three LOD meshes;
 * unknown mappings remain untextured. No associations are guessed. */
int cnk_vita_native_run_with_kart_skins(const char *vfs_path,
    const char *manifest_path,const char *racer_map_path,
    const char *sound_map_path,const char *music_map_path,
    const char *music_directory,const char *driver_map_path,
    const char *kart_skins_path);
#endif
