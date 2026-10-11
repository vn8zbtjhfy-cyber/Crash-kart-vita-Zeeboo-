#include "cnk_vita_diagnostics.h"
#include <stdio.h>

const char *cnk_vita_diagnostic_name(int code){
 switch(code){
 case 0:return "normal_exit";
 case -1:return "source_archive_open";
 case -2:return "race_runtime_init";
 case -3:return "controller_init";
 case -4:return "controller_read";
 case -5:return "race_runtime_tick";
 case -7:return "texture_manifest_read";
 case -8:return "png_catalog";
 case -9:return "scene_texture_binding";
 case -10:return "gpu_texture_init";
 case -11:return "gpu_texture_upload";
 case -12:return "racer_assets";
 case -13:return "display_vblank";
 case -14:return "audio_output_open";
 case -15:return "sound_cue_setup";
 case -16:return "audio_worker_start";
 case -17:return "audio_mixer_lock";
 case -18:return "sound_cue_dispatch";
 case -20:return "audio_worker_failure";
 case -21:return "music_cue_setup";
 case -22:return "music_cue_dispatch";
 case -23:return "racer_attachment";
 case -24:return "driver_bindings";
 case -25:return "driver_attachment";
 case -26:return "gpu_startup";
 case -27:return "character_gallery_load";
 case -28:return "character_gallery_textures";
 case -29:return "kart_skins_load";
 case -30:return "kart_skins_apply";
 case -31:return "source_grid_load";
 case -32:return "item_texture_catalog";
 case -33:return "item_texture_setup";
 case -34:return "item_texture_upload";
 case -35:return "source_boost_pads";
 case -36:return "source_gallery_flags";
 case -37:return "character_gallery_draw";
 case -38:return "source_gallery_flags";
 case -39:return "kart_gallery_load";
 case -40:return "kart_gallery_draw";
 case -41:return "gallery_source_flag";
 case -42:return "gallery_texture_load";
 case -43:return "collision_probe";
 case -44:return "developer_cup";
 case -45:return "developer_story";
 case -46:return "developer_racer_preview";
 case -47:return "game_frame_render";
 case -48:return "gpu_gl_error";
 default:return "unknown_status";
 }
}

int cnk_vita_diagnostic_write(const char *path,int code){
 if(!path||!path[0])return -1;
 FILE *f=fopen(path,"wb");
 if(!f)return -2;
 int rc=fprintf(f,"CNK_NATIVE_STATUS_V1\ncode=%d\nstage=%s\n",
                code,cnk_vita_diagnostic_name(code));
 int close_rc=fclose(f);
 if(rc<0||close_rc!=0)return -3;
 return 0;
}
