/* Native PS Vita race loop. Not an emulator; intentionally unshipped until
 * original material associations, animations, modes and collision logic are
 * recovered, VitaSDK compilation succeeds and console QA is complete.
 * This uses a DEBUG overlay in place of the original Zeebo HUD/menu. */
#include "cnk_vita_native_app.h"
#include "cnk_app_runtime.h"
#include "cnk_vita_controls.h"
#include "cnk_vitagl_game_frame.h"
#include "cnk_vitagl_scene_assets.h"
#include "cnk_item_sprite_map.h"
#include "cnk_texture_manifest.h"
#include "cnk_racer_assets.h"
#include "cnk_kart_skins.h"
#include "cnk_vita_audio_worker.h"
#include "cnk_vita_diagnostics.h"
#include "cnk_vita_telemetry.h"
#include "cnk_sound_cues.h"
#include "cnk_music_cues.h"
#include "cnk_driver_bindings.h"
#include "cnk_results_text.h"
#include "cnk_lang.h"
#include "cnk_verified_characters.h"
#include "cnk_character_gallery.h"
#include "cnk_source_kart_gallery.h"
#include "cnk_gallery_skins.h"
#include "cnk_dev_race_visuals.h"
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>
#include <psp2/io/stat.h>
#include <vitaGL.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* Tiny 5x7 diagnostic typeface. No claim this is Zeebo's original UI. */
typedef struct {char character;uint8_t rows[7];} glyph5;
static const glyph5 chars[]={
 {' ',{0,0,0,0,0,0,0}}, {'0',{14,17,19,21,25,17,14}},
 {'1',{4,12,4,4,4,4,14}}, {'2',{14,17,1,2,4,8,31}},
 {'3',{30,1,1,14,1,1,30}}, {'4',{2,6,10,18,31,2,2}},
 {'5',{31,16,30,1,1,17,14}}, {'6',{6,8,16,30,17,17,14}},
 {'7',{31,1,2,4,8,8,8}}, {'8',{14,17,17,14,17,17,14}},
 {'9',{14,17,17,15,1,2,12}},
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'-',{0,0,0,31,0,0,0}}, {':',{0,4,4,0,4,4,0}},
 {'/',{1,1,2,4,8,16,16}}, {'=',{0,31,0,31,0,0,0}},
 {'!',{4,4,4,4,4,0,4}}, {'?',{14,17,1,2,4,0,4}},
 {'.',{0,0,0,0,0,12,12}}
};
static const uint8_t *glyph(char c){
 if(c>='a'&&c<='z')c=(char)(c-32);
 for(unsigned i=0;i<sizeof(chars)/sizeof(chars[0]);i++)
  if(chars[i].character==c)return chars[i].rows;
 return chars[0].rows;
}
/* TEMPORARY 5x7 diagnostic renderer: Latin accents are approximated by
 * ASCII base letters here ONLY. Authentic UTF-8 remains intact in cnk_lang.
 * Final Zeebo PFNT glyph bitmaps are still unverified/unrendered. */
static char diagnostic_latin(const char *p,unsigned *advance){
 unsigned char a=(unsigned char)p[0],b=(unsigned char)p[1];
 *advance=1;
 if(a<0x80u)return (char)a;
 if(a==0xc2u && b){
  *advance=2;return b==0xa1u?'!':b==0xbfu?'?':' ';
 }
 if(a!=0xc3u||!b)return ' ';
 *advance=2;
 if((b>=0x80u&&b<=0x85u)||(b>=0xa0u&&b<=0xa5u))return 'A';
 if(b==0x87u||b==0xa7u)return 'C';
 if((b>=0x88u&&b<=0x8bu)||(b>=0xa8u&&b<=0xabu))return 'E';
 if((b>=0x8cu&&b<=0x8fu)||(b>=0xacu&&b<=0xafu))return 'I';
 if(b==0x91u||b==0xb1u)return 'N';
 if((b>=0x92u&&b<=0x96u)||(b>=0xb2u&&b<=0xb6u))return 'O';
 if((b>=0x99u&&b<=0x9cu)||(b>=0xb9u&&b<=0xbcu))return 'U';
 if(b==0x9du||b==0xbdu)return 'Y';
 return ' ';
}
static void rect(float x,float y,float w,float h){
 glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);
}
static void text5(float x,float y,float scale,const char *text){
 if(!text)return;
 /* Batch all glyph pixels in one immediate-mode draw call. */
 glBegin(GL_QUADS);
 for(unsigned k=0,i=0;text[i]&&k<112;k++){
  unsigned consumed=1;char shown=diagnostic_latin(text+i,&consumed);
  const uint8_t *g=glyph(shown);
  i+=consumed;
  for(int r=0;r<7;r++)for(int b=0;b<5;b++)
   if(g[r]&(1u<<(4-b)))rect(x+b*scale,y+r*scale,scale,scale);
  x+=6.0f*scale;
 }
 glEnd();
}
/* Line as a tiny quad; works with the existing Vita immediate GL path and
 * avoids relying on platform-dependent thick GL_LINES support. */
static void map_segment(float x,float y,float ax,float ay,float bx,float by){
 float dx=bx-ax,dy=by-ay,len=sqrtf(dx*dx+dy*dy);
 if(len<0.01f)return;
 float nx=-dy*1.15f/len,ny=dx*1.15f/len;
 glVertex2f(x+ax+nx,y+ay+ny);glVertex2f(x+bx+nx,y+by+ny);
 glVertex2f(x+bx-nx,y+by-ny);glVertex2f(x+ax-nx,y+ay-ny);
}
static void debug_minimap(const cnk_minimap_frame *map){
 if(!map||map->path_count<3||map->path_count>CNK_MINIMAP_PATH_POINTS ||
    map->racer_count>CNK_MAX_RACERS)return;
 const float x=775.0f,y=320.0f,size=145.0f;
 glColor4f(0.01f,0.04f,0.10f,0.88f);
 glBegin(GL_QUADS);rect(761,295,174,190);glEnd();
 glColor4f(0.95f,0.93f,0.64f,1);
 text5(776,302,1.35f,"SOURCE ROUTE");
 glColor4f(0.68f,0.80f,0.96f,1);
 glBegin(GL_QUADS);
 for(uint32_t i=0;i<map->path_count;i++){
  cnk_minimap_point a=map->path[i],b=map->path[(i+1)%map->path_count];
  map_segment(x,y,a.x*size,a.z*size,b.x*size,b.z*size);
 }
 glEnd();
 for(uint32_t i=0;i<map->racer_count;i++){
  cnk_minimap_point v=map->racers[i];
  if(i==0)glColor4f(0.25f,1.0f,0.37f,1);
  else glColor4f(1.0f,0.49f,0.33f,1);
  glBegin(GL_QUADS);rect(x+size*v.x-3.0f,y+size*v.z-3.0f,6,6);glEnd();
 }
}
static void debug_overlay(const cnk_app_snapshot *snap,int draw_rc,int gallery_on,
                          int kart_gallery_on,unsigned kart_lod,const cnk_lang_pack *lang,
                          int dev_visual_preview){
 char a[120],b[120],c[120];
 if(snap->state==CNK_FRONTEND_SELECT){
  const char *track=cnk_lang_text(lang,29); /* Zeebo LANG ordinal: SELECT TRACK */
  snprintf(a,sizeof(a),"%s %02u/%02u",track?track:"ZEEBO RACE SCENE",
           snap->selected_race+1,snap->available_races);
  snprintf(b,sizeof(b),"SOURCE RESOURCE %u  LANGUAGE %s - SELECT TO CHANGE",
           snap->source_resource_id,lang && lang->storage?lang->name:"UNAVAILABLE");
  snprintf(c,sizeof(c),"LEFT RIGHT TRACK  SQUARE LAPS %u  TRIANGLE %s  CROSS RACE%s",
           snap->configured_laps,snap->practice_mode?"NORMAL":"SOLO",
           snap->story_available && !snap->practice_mode?"  START STORY DEV":
           snap->cup_available && !snap->practice_mode?"  START CUP DEV":"");
 }else{
  const char *lap=cnk_lang_text(lang,31); /* Zeebo LANG ordinal: LAP */
  const char *rank=cnk_lang_text(lang,30); /* Zeebo LANG ordinal: POSITION */
  snprintf(a,sizeof(a),"%s %u/%u  %s %u/%u",lap?lap:"LAP",
           snap->hud.lap,snap->hud.total_laps,rank?rank:"RANK",
           snap->hud.rank,snap->hud.competitors);
  snprintf(b,sizeof(b),"TIME %llu.%03llu  LAST %llu.%03llu  BEST %llu.%03llu",
           (unsigned long long)(snap->hud.elapsed_ms/1000),
           (unsigned long long)(snap->hud.elapsed_ms%1000),
           (unsigned long long)(snap->hud.last_lap_ms/1000),
           (unsigned long long)(snap->hud.last_lap_ms%1000),
           (unsigned long long)(snap->hud.best_lap_ms/1000),
           (unsigned long long)(snap->hud.best_lap_ms%1000));
  if(snap->state==CNK_FRONTEND_PAUSE)
   snprintf(c,sizeof(c),"%s  START %s  CIRCLE MENU",
    cnk_lang_text(lang,56)?cnk_lang_text(lang,56):"PAUSE",
    cnk_lang_text(lang,57)?cnk_lang_text(lang,57):"RESUME");
  else if(snap->state==CNK_FRONTEND_RESULTS){
   if(snap->story_active)
    snprintf(c,sizeof(c),"DEV STORY %u/%u  %s  CROSS %s",
     snap->story_completed,snap->story_total,
     snap->story_last_attempt_success?"GOAL PASSED":"RETRY OBJECTIVE",
     snap->story_completed==snap->story_total?"FINAL":"NEXT STAGE");
   else if(snap->cup_active)
    snprintf(c,sizeof(c),"PROVISIONAL CUP %u/%u  CROSS %s  CIRCLE EXIT",
     snap->cup_standings.completed_rounds,snap->cup_standings.total_rounds,
     snap->cup_standings.finished?"FINAL TABLE":"NEXT TRACK");
   else
    snprintf(c,sizeof(c),"%s  CROSS %s  CIRCLE MENU",
     cnk_lang_text(lang,35)?cnk_lang_text(lang,35):"RESULTS",
     cnk_lang_text(lang,58)?cnk_lang_text(lang,58):"RESTART");
  }
  else if(snap->start_phase==CNK_START_THREE ||
          snap->start_phase==CNK_START_TWO ||
          snap->start_phase==CNK_START_ONE)
   snprintf(c,sizeof(c),"START SIGNAL - KARTS LOCKED UNTIL GO");
  else snprintf(c,sizeof(c),"CROSS GAS  SQUARE BRAKE REVERSE  SPEED %.1f %s",
                snap->hud.kart_speed,snap->hud.reversing?"REVERSE":"FORWARD");
 }
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
 glOrtho(0,960,544,0,-1,1);
 glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
 glDisable(GL_DEPTH_TEST);glDisable(GL_TEXTURE_2D);
 /* A cursed-candle victim sees a deliberately provisional dark overlay.
  * No original Zeebo fade/color/particles are asserted here. Never hide HUD. */
 if(snap->candle_affects_player && snap->state==CNK_FRONTEND_RACE){
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(0.0f,0.0f,0.0f,0.82f);
  glBegin(GL_QUADS);rect(0,0,960,544);glEnd();
  glDisable(GL_BLEND);
 }
 glColor4f(0.01f,0.04f,0.08f,1.0f);
 glBegin(GL_QUADS);rect(0,0,960,102);rect(0,498,960,46);glEnd();
 glColor4f(0.9f,0.95f,1.0f,1.0f);
 text5(18,10,2.0f,"CRASH NITRO KART 3D - NATIVE DEVELOPMENT");
 text5(18,34,2.0f,a);
 text5(18,58,2.0f,b);
 /* A completed, authenticated DEV Story can be reviewed after restarting.
  * No fabricated original Zeebo campaign chapters are implied. */
 if(snap->state==CNK_FRONTEND_SELECT && snap->has_completed_story){
  char last[120];
  snprintf(last,sizeof(last),"STORY DEV COMPLETE  %u STAGES - LAST RANK %u  SAVED",
           snap->completed_story_count,
           (unsigned)snap->completed_story_ranks[snap->completed_story_count-1u]);
  glColor4f(0.61f,1.0f,0.73f,1);
  text5(18,80,1.5f,last);
  glColor4f(0.9f,0.95f,1.0f,1);
 }
 text5(18,509,2.0f,c);
 if(snap->has_personal_best){
  char pb[96];
  snprintf(pb,sizeof(pb),"LOCAL %s PB LAP %llu.%03llu  RACE %llu.%03llu",
   snap->practice_mode?"SOLO":"NORMAL",
   (unsigned long long)(snap->personal_best.best_lap_ms/1000),
   (unsigned long long)(snap->personal_best.best_lap_ms%1000),
   (unsigned long long)(snap->personal_best.best_race_ms/1000),
   (unsigned long long)(snap->personal_best.best_race_ms%1000));
  glColor4f(0.5f,1.0f,0.7f,1);
  text5(25,483,1.25f,pb);
 }
 /* Source-asset viewer: names supported by Zeebo model evidence. This
  * does NOT imply the displayed driver belongs to the selected race slot. */
 if(gallery_on && snap->state==CNK_FRONTEND_SELECT){
  glColor4f(0.01f,0.04f,0.08f,0.84f);
  glBegin(GL_QUADS);rect(6,432,948,54);glEnd();
  glColor4f(0.95f,0.89f,0.6f,1.0f);
  text5(22,442,1.55f,"CRASH BANDICOOT");
  text5(352,442,1.55f,"COCO BANDICOOT");
  text5(684,442,1.55f,"RIPPER ROO");
  glColor4f(0.75f,0.83f,0.9f,1.0f);
  text5(225,468,1.05f,"L ORBIT  -  ORIGINAL MODELS  -  NO DRIVER/KART ASSIGNMENTS");
 }
 /* Development viewer: these are six original 3DOB vehicle families,
  * deliberately UNTEXTURED; the source character/kart pairs are unknown. */
 if(kart_gallery_on && snap->state==CNK_FRONTEND_SELECT){
  static const char *family[6]={"NEOSPRINT01","LEMANS02","COCSTER2",
      "RALLYE","CRASHSTANG128","NEOMINI"};
  glColor4f(0.95f,0.9f,0.7f,1);
  for(unsigned i=0;i<6;i++)
   text5((float)(i%3u)*320+20.0f,i<3?250.f:425.f,1.35f,family[i]);
  char detail_text[120];
  snprintf(detail_text,sizeof(detail_text),
     "SOURCE TEXL LABELS  -  L ORBIT  R DETAIL %s  -  NO DRIVER MATCH",
     kart_lod==0?"HIGH":kart_lod==1?"MED":"LOW");
  glColor4f(0.85f,0.87f,0.95f,1);
  text5(18,470,1.18f,detail_text);
 }
 /* Development-only drift meter, driven by actual 60Hz race simulation.
  * Zeebo's original graphics and boost coefficients are still unknown. */
 if(snap->state==CNK_FRONTEND_RACE && snap->has_hud){
  glColor4f(0.28f,0.28f,0.31f,1.0f);
  glBegin(GL_QUADS);rect(690,70,244,15);glEnd();
  if(snap->boost_active)glColor4f(0.25f,1.0f,0.35f,1.0f);
  else glColor4f(0.98f,0.67f,0.16f,1.0f);
  glBegin(GL_QUADS);
  rect(692,72,(snap->boost_active?1.0f:snap->boost_charge_fraction)*240.0f,11);
  glEnd();
  glColor4f(1.0f,1.0f,1.0f,1.0f);
  text5(690,89,1.0f,snap->boost_active?"BOOST":
        (snap->player_boost.drifting?"DRIFT CHARGING":"R TO DRIFT"));
 }
 if(snap->state==CNK_FRONTEND_RACE && snap->has_hud){
  char label[90];
  const char *weapon=snap->player_items.held==CNK_WEAPON_BOMB?"BOMB":
      snap->player_items.held==CNK_WEAPON_TURBO?"TURBO":
      snap->player_items.held==CNK_WEAPON_MISSILE?"MISSILE":
      snap->player_items.held==CNK_WEAPON_TNT?"TNT":
      snap->player_items.held==CNK_WEAPON_OIL?"OIL":
      snap->player_items.held==CNK_WEAPON_SHIELD?"SHIELD":
      snap->player_items.held==CNK_WEAPON_TROPY_CLOCK?"TROPY CLOCK":
      snap->player_items.held==CNK_WEAPON_CURSED_CANDLE?"CURSED CANDLE":
      snap->player_items.held==CNK_WEAPON_AKU_AKU?"AKU AKU":
      snap->player_items.held==CNK_WEAPON_KART_JACKING?"KART JACKING":"EMPTY";
  snprintf(label,sizeof(label),"L ITEM %s  WUMPA %u/10",weapon,
           (unsigned)snap->player_items.wumpa);
  glColor4f(0.95f,0.9f,0.45f,1.0f);text5(610,112,1.3f,label);
  if(snap->placed_pads){
   snprintf(label,sizeof(label),"SOURCE-REVIEWED BOOST PADS %u",snap->placed_pads);
   text5(610,196,1.0f,label);
  }
  snprintf(label,sizeof(label),"MISSILES %u TRAPS %u %s",
    snap->active_missiles,snap->active_hazards,
    snap->player_items.shield_seconds>0?"SHIELD ACTIVE":
    snap->player_items.stun_seconds>0?"STUN":
    snap->player_items.oil_seconds>0?"OIL SLOW":"");
  glColor4f(0.9f,0.9f,0.9f,1.0f);text5(610,130,1.0f,label);
  if(snap->clock_seconds>0.0f){
   snprintf(label,sizeof(label),"TROPY CLOCK %.1fs %s",snap->clock_seconds,
             snap->clock_affects_player?"SLOWED":"RIVALS SLOWED");
   glColor4f(1.0f,0.8f,0.25f,1.0f);text5(610,211,1.0f,label);
  }
  if(snap->candle_seconds>0.0f){
   snprintf(label,sizeof(label),"CURSED CANDLE %.1fs %s",snap->candle_seconds,
             snap->candle_affects_player?"DARKNESS":"RIVALS AFFECTED");
   glColor4f(0.9f,0.6f,1.0f,1.0f);text5(610,224,1.0f,label);
  }
  snprintf(label,sizeof(label),"CIRCLE RECOVER  USED %u",snap->hud.recoveries);
  text5(610,146,1.0f,label);
  if(snap->hud.last_sector_gate!=UINT32_MAX){
   snprintf(label,sizeof(label),"SECTOR %u  LAST %llu.%03llu  BEST %llu.%03llu",
     snap->hud.last_sector_gate,
     (unsigned long long)(snap->hud.last_sector_ms/1000),
     (unsigned long long)(snap->hud.last_sector_ms%1000),
     (unsigned long long)(snap->hud.best_sector_ms/1000),
     (unsigned long long)(snap->hud.best_sector_ms%1000));
   text5(610,160,0.95f,label);
  }
 }
 if(dev_visual_preview && snap->state==CNK_FRONTEND_RACE){
  glColor4f(1.0f,0.6f,0.2f,1.0f);
  text5(30.0f,506.0f,1.15f,"DEV SOURCE MESHES - PROVISIONAL PILOT ASSIGNMENT");
 }
 /* Recovered FINAL results of a completed, source-validated developer Cup.
  * This is deliberately a diagnostic scoreboard, NOT original Zeebo art.
  * Never show incomplete Cup scores as historical final standings. */
 if(snap->state==CNK_FRONTEND_SELECT && snap->has_completed_cup &&
    !snap->cup_active && snap->completed_cup_standings.finished){
  glColor4f(0.04f,0.07f,0.12f,0.93f);
  glBegin(GL_QUADS);rect(575,178,348,35+38.0f*snap->completed_cup_standings.count);glEnd();
  glColor4f(1.0f,0.86f,0.3f,1.0f);
  text5(592,190,1.35f,"LAST COMPLETED CUP (DEV)");
  for(uint32_t i=0;i<snap->completed_cup_standings.count;i++){
   const cnk_cup_row *r=&snap->completed_cup_standings.rows[i];
   char line[90];
   snprintf(line,sizeof(line),"%u. KART %u     %u POINTS",r->rank,r->slot+1u,r->points);
   glColor4f(r->slot==0u?0.45f:0.9f,1.0f,0.83f,1.0f);
   text5(592,224.0f+38.0f*i,1.3f,line);
  }
 }
 /* Provisional alert: actual item positions + velocities, not source art.
  * The warning can clear as soon as a missile steers away or expires. */
 if(snap->state==CNK_FRONTEND_RACE && snap->has_hud &&
    snap->incoming_projectile){
  char warning[90];
  snprintf(warning,sizeof(warning),"INCOMING %s %u MS",
    snap->incoming_kind==CNK_WEAPON_MISSILE?"MISSILE":"BOMB",
    snap->incoming_eta_ms);
  glColor4f(1.0f,0.42f,0.16f,1.0f);
  text5(600,180,1.7f,warning);
 }
 /* Source-backed development minimap drawn above the track render.
  * Original HUD art, rotation, scaling and styles are not yet recovered. */
 if(snap->has_hud && snap->minimap.path_count)debug_minimap(&snap->minimap);
 /* Advisory from the original DRV0 path geometry. This provisional
  * banner is NOT the original Zeebo wrong-way graphic or audio cue. */
 if(snap->state==CNK_FRONTEND_RACE && snap->has_hud && snap->hud.wrong_way){
  glColor4f(0.25f,0.01f,0.02f,0.92f);
  glBegin(GL_QUADS);rect(333,163,294,49);glEnd();
  glColor4f(1.0f,0.3f,0.15f,1.0f);
  text5(353,177,3.0f,"WRONG WAY");
 }
 /* Debug start board, deliberately not represented as original Zeebo art.
  * It is driven by the same phase that gates movement and race time. */
 if(snap->state==CNK_FRONTEND_RACE &&
    (snap->start_phase==CNK_START_THREE ||
     snap->start_phase==CNK_START_TWO ||
     snap->start_phase==CNK_START_ONE ||
     snap->start_phase==CNK_START_GO)){
  const char *message=snap->start_phase==CNK_START_THREE?"3":
     snap->start_phase==CNK_START_TWO?"2":
     snap->start_phase==CNK_START_ONE?"1":"GO";
  float scale=snap->start_phase==CNK_START_GO?10.0f:12.0f;
  float width=(float)strlen(message)*6.0f*scale;
  glColor4f(0.02f,0.05f,0.1f,1.0f);
  glBegin(GL_QUADS);rect(367,185,226,160);glEnd();
  if(snap->start_phase==CNK_START_GO)glColor4f(0.2f,1.0f,0.3f,1.0f);
  else glColor4f(1.0f,0.83f,0.15f,1.0f);
  text5((960.0f-width)*0.5f,218.0f,scale,message);
 }
 /* These are actual computed ranks/timings, not fabricated finish times;
  * unfinished rivals stay labelled RACING. Not the original Zeebo menu. */
 if(snap->state==CNK_FRONTEND_RESULTS && snap->classification.count){
  cnk_results_line lines[CNK_MAX_RACERS]={0};
  int count=cnk_results_text_build(&snap->classification,lines);
  if(count>0){
   glColor4f(0.02f,0.05f,0.1f,0.97f);
   glBegin(GL_QUADS);rect(145,107,670,377);glEnd();
   glColor4f(1.0f,0.89f,0.4f,1.0f);
   text5(190,125,snap->cup_active?2.0f:2.6f,"RACE RESULTS - DEVELOPMENT");
   if(snap->cup_active){
    glColor4f(0.98f,0.76f,0.24f,1.0f);
    text5(685,125,1.5f,"CUP POINTS");
   }
   for(int i=0;i<count;i++){
    float y=178.0f+(float)i*47.0f;
    if(lines[i].local_player)glColor4f(0.35f,1.0f,0.35f,1.0f);
    else glColor4f(0.95f,0.95f,1.0f,1.0f);
    text5(180,y,1.65f,lines[i].primary);
    glColor4f(0.75f,0.85f,0.94f,1.0f);
    text5(200,y+19.0f,1.25f,lines[i].secondary);
    if(snap->cup_active){
     for(uint32_t j=0;j<snap->cup_standings.count;j++){
      const cnk_cup_row *row=&snap->cup_standings.rows[j];
      if(row->slot==snap->classification.rows[i].racer_slot){
       char score[40];
       snprintf(score,sizeof(score),"%u PTS",row->points);
       glColor4f(1.0f,0.82f,0.29f,1.0f);
       text5(713,y,1.45f,score);
       break;
      }
     }
    }
   }
  }
 }
 if(draw_rc) {glColor4f(1.0f,0.4f,0.4f,1.0f);
  text5(18,80,2.0f,"RENDER ERROR");}
 else {glColor4f(1.0f,0.8f,0.3f,1.0f);
  text5(18,80,1.5f,"UNFINISHED PORT - NOT ORIGINAL HUD");}
 glMatrixMode(GL_MODELVIEW);glPopMatrix();
 glMatrixMode(GL_PROJECTION);glPopMatrix();
 glMatrixMode(GL_MODELVIEW);
}

/* The callbacks take exactly the same mutex as the SceAudioOut worker. */
static unsigned resolve_item_texture(void *ptr,cnk_item_sprite sprite){
 return cnk_vitagl_textures_resolve(ptr,(uint16_t)sprite);
}
static int music_lock(void *ctx){
 return cnk_vita_audio_worker_lock((cnk_vita_audio_worker *)ctx);
}
static int music_unlock(void *ctx){
 return cnk_vita_audio_worker_unlock((cnk_vita_audio_worker *)ctx);
}
int cnk_vita_native_run_with_kart_skins(const char *vfs_path,
                const char *manifest_path,const char *racer_map_path,
                const char *sound_map_path,const char *music_map_path,
                const char *music_directory,const char *driver_map_path,
                const char *kart_skins_path){
 fufs_file vfs={0};
 cnk_app_runtime app={0};
 cnk_app_snapshot snapshot={0};
 cnk_lang_pack native_lang={0};
 cnk_item_layout item_layout={0};
 cnk_pad_layout pad_layout={0};
 int result=-1;
 int gpu_started=0;
 cnk_vita_telemetry perf={0};
 cnk_texture_manifest manifest={0};
 cnk_png_catalog png={0};
 cnk_racer_assets racer_assets={0};
 /* This GPU-owned table is ~74 KiB: keep it off the Vita main stack. */
 cnk_kart_skins *kart_skins=NULL;
 cnk_racer_visual_attachment visuals[CNK_MAX_RACERS]={{0}};
 cnk_racer_visual_attachment drivers[CNK_MAX_RACERS]={{0}};
 cnk_driver_bindings driver_bindings={0};
 cnk_character_pack character_pack={0};
 cnk_source_kart_gallery kart_gallery={0};
 cnk_dev_race_visuals preview_karts={0};
 int preview_active=0;
 cnk_gallery_skins *gallery_skins=NULL;
 int have_drivers=0;
 int have_gallery=0,have_kart_gallery=0;
 unsigned gallery_lod=1u;
 int gallery_r_pressed=0, gallery_l_pressed=0, lang_pressed=0;
 unsigned gallery_angle_step=0;
 cnk_vitagl_textures track_textures={0};
 cnk_vitagl_textures item_textures={0};
 int item_textures_active=0;
 cnk_vita_audio_worker audio={0};
 cnk_sound_cues cues={0};
 cnk_music_cues music={0};
 int audio_initialized=0, audio_started=0;
 uint32_t loaded_scene=UINT32_MAX;
 int textures_active=0;
 /* Write concise error breadcrumbs even if the source archive cannot be
  * opened; this also makes a missing app0:/data.vfs diagnosable on Vita. */
 (void)sceIoMkdir("ux0:data",0777);
 if(!vfs_path || fufs_open(vfs_path,&vfs)){
  (void)cnk_vita_telemetry_write("ux0:data/cnk3d_perf.txt",&perf,-1);
  (void)cnk_vita_diagnostic_write("ux0:data/cnk3d_last_status.txt",-1);
  fprintf(stderr,"CNK Vita: -1 %s\n",cnk_vita_diagnostic_name(-1));
  return -1;
 }
 /* The original LANG contains five authentic source translations, including
  * Spanish. We use a Spanish developer overlay by default; this does NOT
  * replace the not-yet-decoded Zeebo bitmap font or original screen layouts.
  * Missing/corrupt source text never prevents a race. */
 cnk_lang_locale source_locale=CNK_LANG_SPANISH;
 /* Prefer writable per-user choice. Legacy shipped read-only preference is
  * supported only when no valid user setting is available. */
 if(cnk_lang_preference_read("ux0:data/cnk3d_language.txt",&source_locale) &&
    cnk_lang_preference_read("app0:/source_language.txt",&source_locale))
  source_locale=CNK_LANG_SPANISH;
 if(cnk_lang_load(&vfs,source_locale,&native_lang))
  fprintf(stderr,"CNK: original LANG unavailable; diagnostic labels only\n");
 if(cnk_app_runtime_init(&app,&vfs,3,2)){result=-2;goto cleanup;}
 /* Optional DEV-only cup: authentic SCEN resource IDs, user-specified points.
  * A missing file never enables guessed cups. START on SELECT begins it.
  * This is NOT a recovered Zeebo cup roster or scoring table. */
 cnk_cup_manifest dev_cup={0};
 int dev_cup_rc=cnk_cup_manifest_read("app0:/developer_cup.tsv",
                         &app.frontend.catalog,app.frontend.racer_count,&dev_cup);
 if(dev_cup_rc && dev_cup_rc!=-2){result=-44;goto cleanup;}
 if(!dev_cup_rc && cnk_frontend_cup_configure(&app.frontend,&dev_cup)){
  result=-44;goto cleanup;
 }

 /* Optional DEV-only story objectives, using authentic circuit resource IDs.
  * No historical Zeebo chapter/track objectives are claimed. The normal
  * game has no Story until the user supplies a verified developer manifest. */
 cnk_story_manifest dev_story={0};
 int dev_story_rc=cnk_story_manifest_read("app0:/developer_story.tsv",
              &app.frontend.catalog,app.frontend.racer_count,&dev_story);
 if(dev_story_rc && dev_story_rc!=-2){result=-45;goto cleanup;}
 if(!dev_story_rc && cnk_frontend_story_configure(&app.frontend,&dev_story)){
  result=-45;goto cleanup;
 }

 /* Developer-only opt-in collision experiment. Source Zeebo flags/materials
  * are not yet proven, so the default remains OFF on every clean install. */
 cnk_collision_probe probe_mode={0};
 int probe_rc=cnk_collision_probe_read("app0:/collision_probe.tsv",&probe_mode);
 if(probe_rc && probe_rc!=-2){result=-43;goto cleanup;}
 if(!probe_rc && cnk_app_runtime_set_collision_probe(&app,probe_mode)){
  result=-43;goto cleanup;
 }

 /* PS Vita writable user storage. Missing/corrupt saves never block racing;
  * a corrupt file is intentionally not overwritten unless explicitly fixed. */
 if(cnk_app_runtime_set_records_path(&app,"ux0:data/cnk3d_records.dat"))
  fprintf(stderr,"CNK: local records unavailable; race continues without writes\n");
 /* Only consume explicitly curated positions for the correct FUFS scene ID.
  * No invented default item locations. Absence is a valid state. */
 int layout_rc=cnk_item_layout_read("app0:/verified_item_placements.tsv",&item_layout);
 if(layout_rc && layout_rc!=-2){result=-31;goto cleanup;}
 if(!layout_rc && cnk_app_runtime_set_item_layout(&app,&item_layout)){
  result=-31;goto cleanup;
 }
 int pads_rc=cnk_pad_layout_read("app0:/verified_boost_pads.tsv",&pad_layout);
 if(pads_rc && pads_rc!=-2){result=-35;goto cleanup;}
 if(!pads_rc && cnk_app_runtime_set_pad_layout(&app,&pad_layout)){
  result=-35;goto cleanup;
 }
 if(manifest_path && *manifest_path){
  int map_status=cnk_texture_manifest_read(manifest_path,&manifest);
  if(map_status && map_status!=-2){result=-7;goto cleanup;}
  if(manifest.count){
   if(cnk_png_catalog_build(&vfs,&png) ||
      cnk_texture_manifest_verify_all(&manifest,&vfs,&png)){
    result=-8;goto cleanup;
   }
  }
 }
 if(racer_map_path && *racer_map_path){
  int loaded=cnk_racer_assets_load(racer_map_path,&vfs,&racer_assets);
  if(loaded && loaded!=-2){result=-12;goto cleanup;}
  if(!loaded)cnk_racer_assets_attachments(&racer_assets,visuals);
 }
 if(driver_map_path && *driver_map_path){
  int loaded=cnk_driver_bindings_load(driver_map_path,&driver_bindings);
  if(loaded && loaded!=-2){result=-24;goto cleanup;}
  if(!loaded){
   /* Reject an unsourced "floating" driver without a kart counterpart. */
   for(unsigned i=0;i<CNK_MAX_RACERS;i++)
    if(driver_bindings.assigned[i]&&!racer_assets.assigned[i]){
     result=-25;goto cleanup;
    }
   have_drivers=driver_bindings.count!=0;
  }
 }
 int have_kart_skins=0;
 if(kart_skins_path && *kart_skins_path){
  FILE *skin_probe=fopen(kart_skins_path,"rb");
  if(skin_probe){have_kart_skins=1;fclose(skin_probe);}
  else if(errno!=ENOENT){result=-29;goto cleanup;}
 }
 /* Playable DEVELOPMENT preview: authentic six source meshes are used as
  * *unverified visual placeholders* only for racer slots that lack reviewed
  * associations. Without this, the regular race is often just an empty
  * circuit because no authentic kart-to-pilot bindings are installed.
  * The final verified game must supply those bindings and textures.
  * A diagnostic can opt out via app0:/disable_dev_racer_fallback.flag.
  * The legacy developer_racer_preview.flag remains accepted, but the
  * presence of that file alone is no longer necessary in DEV builds. */
 int need_preview=0;
 for(unsigned i=0;i<app.frontend.racer_count;i++)
  if(!racer_assets.assigned[i])need_preview=1;
 FILE *disable_preview=fopen("app0:/disable_dev_racer_fallback.flag","rb");
 if(disable_preview){fclose(disable_preview);need_preview=0;}
 else if(errno!=ENOENT){result=-46;goto cleanup;}
 if(need_preview){
  if(cnk_dev_race_visuals_load(&vfs,&preview_karts)){
   /* A corrupted preview cannot stop a source-validated race from loading;
    * it merely leaves the optional visual fallback unavailable. */
   fprintf(stderr,"CNK: unverified kart preview unavailable\n");
  }else preview_active=1;
 }
 /* Optional development gallery of three independently identified source
  * characters. The opt-in marker never implies a source kart assignment. */
 FILE *gallery_probe=fopen("app0:/source_character_gallery.flag","rb");
 if(gallery_probe){have_gallery=1;fclose(gallery_probe);}
 else if(errno!=ENOENT){result=-36;goto cleanup;}
 FILE *kart_gallery_probe=fopen("app0:/source_kart_gallery.flag","rb");
 if(kart_gallery_probe){have_kart_gallery=1;fclose(kart_gallery_probe);}
 else if(errno!=ENOENT){result=-38;goto cleanup;}
 /* Two optional gallery markers can coexist. The vehicle viewer takes
  * precedence on SELECT, without loading unused character textures. */
 if(have_kart_gallery)have_gallery=0;
 /* The optional gallery PNG sidecar is never auto-generated from TEXL.
  * Absent sidecar => the original six meshes remain deliberately untextured. */
 int have_gallery_skins=0;
 if(have_kart_gallery){
  FILE *p=fopen("app0:/verified_gallery_textures.tsv","rb");
  if(p){have_gallery_skins=1;fclose(p);}
  else if(errno!=ENOENT){result=-41;goto cleanup;}
 }
 if((have_kart_skins||have_drivers||have_gallery||have_gallery_skins) && !png.items &&
    cnk_png_catalog_build(&vfs,&png)){
  result=-26;goto cleanup;
 }
 /* Neither audio mapping is guessed. Each verified sidecar is optional.
  * The music is converted from the original Zeebo MP3s, but what plays on
  * which scene must be confirmed independently. */
 int have_sound=0,have_music=0;
 FILE *probe=NULL;
 if(sound_map_path && *sound_map_path){
  probe=fopen(sound_map_path,"rb");
  if(probe){have_sound=1;fclose(probe);}
 }
 if(music_map_path && *music_map_path){
  probe=fopen(music_map_path,"rb");
  if(probe){have_music=1;fclose(probe);}
 }
 if(have_sound||have_music){
  if(cnk_vita_audio_worker_open(&audio)){result=-14;goto cleanup;}
  audio_initialized=1;
  if(have_sound && cnk_sound_cues_read(&cues,&audio.output.mixer,&vfs,sound_map_path)){
   result=-15;goto cleanup;
  }
  if(have_music && cnk_music_cues_read(&music,&audio.output.mixer,
                 music_map_path,music_directory)){
   result=-21;goto cleanup;
  }
  if(cnk_vita_audio_worker_start(&audio)){result=-16;goto cleanup;}
  audio_started=1;
 }
 if(cnk_vita_controls_init()<0){result=-3;goto cleanup;}
 vglInit(0x800000u);
 gpu_started=1;
 /* Optional verified original PNG indices for live item actors. No entry
  * = explicit untextured development markers, never a fake Zeebo sprite. */
 FILE *item_probe=fopen("app0:/verified_item_textures.tsv","rb");
 if(item_probe){
  fclose(item_probe);
  cnk_item_sprite_map sprite_map={0};
  if(!png.items && cnk_png_catalog_build(&vfs,&png)){result=-32;goto cleanup;}
  if(cnk_item_sprite_map_read("app0:/verified_item_textures.tsv",&vfs,&png,&sprite_map)){
   result=-32;goto cleanup;
  }
  if(sprite_map.count){
   if(cnk_vitagl_textures_init(&item_textures,&vfs,&png,CNK_ITEM_SPRITE_COUNT)){
    result=-33;goto cleanup;
   }
   item_textures_active=1;
   for(unsigned slot=0;slot<CNK_ITEM_SPRITE_COUNT;slot++)if(sprite_map.present[slot]){
    if(cnk_vitagl_textures_upload(&item_textures,sprite_map.binding[slot])){
     result=-34;goto cleanup;
    }
   }
  }
 }else if(errno!=ENOENT){result=-32;goto cleanup;}
 if(have_kart_skins){
  kart_skins=(cnk_kart_skins*)calloc(1,sizeof(*kart_skins));
  if(!kart_skins){result=-29;goto cleanup;}
  int skin_rc=cnk_kart_skins_load(kart_skins_path,&vfs,&png,&racer_assets,kart_skins);
  if(skin_rc && skin_rc!=-2){result=-29;goto cleanup;}
 }
 if(have_kart_gallery && cnk_source_kart_gallery_load(&vfs,&kart_gallery)){
  result=-39;goto cleanup;
 }
 if(have_gallery_skins){
  gallery_skins=calloc(1,sizeof(*gallery_skins));
  if(!gallery_skins || cnk_gallery_skins_load("app0:/verified_gallery_textures.tsv",
          &vfs,&png,&kart_gallery,gallery_skins)){
   result=-42;goto cleanup;
  }
 }
 if(have_drivers||have_gallery){
  if(cnk_character_pack_load(&vfs,&png,&character_pack)){
   result=-27;goto cleanup;
  }
  if(have_drivers && cnk_driver_bindings_attachments(&driver_bindings,&character_pack,
                                      racer_assets.assigned,drivers)){
   result=-28;goto cleanup;
  }
 }
 /* Opt-in persistent developer Cup. Validate the manifest, source track IDs
  * and all previously recorded finishing positions before auto-resuming.
  * A corrupt checkpoint is left untouched; races stay available normally. */
 if(app.frontend.cup_manifest_ready &&
    cnk_app_runtime_set_cup_checkpoint_path(&app,"ux0:data/cnk3d_cup.dat"))
  fprintf(stderr,"CNK: Cup checkpoint unavailable/corrupt; retaining original file and continuing\n");
 if(app.frontend.story_manifest_ready &&
    cnk_app_runtime_set_story_checkpoint_path(&app,"ux0:data/cnk3d_story.dat"))
  fprintf(stderr,"CNK: Story checkpoint unavailable/corrupt; original file retained\n");
 for(;;){
  cnk_driver_input input={0};uint32_t actions=0;int wants_exit=0;
  if(cnk_vita_controls_read_actions(&input,&actions,&wants_exit)){
   result=-4;break;
  }
  /* SELECT ignores L/R within the existing frontend. Use R edge to
   * cycle ORIGINAL mesh LOD for source viewer, without touching races. */
  if(have_kart_gallery && app.frontend.state==CNK_FRONTEND_SELECT){
   int pressed=(actions&CNK_UI_DRIFT)!=0;
   if(pressed && !gallery_r_pressed)
    gallery_lod=(gallery_lod+1u)%CNK_SOURCE_KART_LODS;
   gallery_r_pressed=pressed;
  }else gallery_r_pressed=0;
  /* L steps around authentic source meshes for visual inspection only.
   * This is NOT a recovered Zeebo character/vehicle animation. */
  if((have_kart_gallery||have_gallery) && app.frontend.state==CNK_FRONTEND_SELECT){
   int pressed=(actions&CNK_UI_ITEM)!=0;
   if(pressed && !gallery_l_pressed)
    gallery_angle_step=(gallery_angle_step+1u)%12u;
   gallery_l_pressed=pressed;
  }else gallery_l_pressed=0;
  /* SELECT changes ONLY the development selector's source LANG pack.
   * Avoid forwarding this adapter-only bit to the strict race frontend.
   * START+SELECT remains exit and never writes a preference. */
  int select_pressed=(actions&CNK_UI_LANGUAGE)!=0;
  if(select_pressed && !lang_pressed && !wants_exit &&
     app.frontend.state==CNK_FRONTEND_SELECT){
   if(cnk_lang_cycle(&vfs,&native_lang)==0){
    if(cnk_lang_preference_write("ux0:data/cnk3d_language.txt",
                                 (cnk_lang_locale)native_lang.locale))
     fprintf(stderr,"CNK: source language active, but could not save preference\n");
   }else fprintf(stderr,"CNK: source language switch failed; previous language retained\n");
  }
  lang_pressed=select_pressed;
  actions &= ~((uint32_t)CNK_UI_LANGUAGE);
  if(wants_exit){result=0;break;}
  uint64_t clock_us=sceKernelGetProcessTimeWide();
  if(cnk_app_runtime_tick(&app,clock_us,actions,input,&snapshot)){
   result=-5;break;
  }
  if(audio_started){
   if(cnk_vita_audio_worker_status(&audio)){result=-20;break;}
   /* WAV file I/O is staged by the music router WITHOUT the mixer lock;
    * only the voice swap and PCM release run inside that mutex. */
   if(have_music){
    unsigned changed=0;
    if(cnk_music_cues_dispatch(&music,&snapshot,music_lock,music_unlock,
                               &audio,&changed)){result=-22;break;}
   }
   if(have_sound){
    unsigned played=0;
    if(cnk_vita_audio_worker_lock(&audio)){result=-17;break;}
    int cue_rc=cnk_sound_cues_dispatch(&cues,&snapshot,&played);
    int unlock_rc=cnk_vita_audio_worker_unlock(&audio);
    if(cue_rc||unlock_rc){result=-18;break;}
   }
  }
  /* Tracks are loaded on a frontend transition. GPU resources belong to
   * exactly one current scene. Never keep stale GL handles after switching. */
  if(!app.frontend.game.loaded ||
     (textures_active && loaded_scene!=app.frontend.game.scene_index)){
   if(textures_active)cnk_vitagl_textures_free(&track_textures);
   textures_active=0;loaded_scene=UINT32_MAX;
  }
  if(app.frontend.game.loaded && manifest.count && !textures_active){
   cnk_game *g=&app.frontend.game;
   size_t numrefs=0;
   size_t ref_capacity=g->scene.texture_count;
   if(ref_capacity>1024u){result=-9;break;}
   cnk_texture_reference *refs=ref_capacity?
       calloc(ref_capacity,sizeof(*refs)):NULL;
   if(ref_capacity && !refs){result=-9;break;}
   if(cnk_texture_manifest_scene(&manifest,&g->scene,g->scene_index,&vfs,&png,
       refs,ref_capacity,&numrefs)){
    free(refs);result=-9;break;
   }
   if(g->scene.texture_count &&
      cnk_vitagl_textures_init(&track_textures,&vfs,&png,g->scene.texture_count)){
    free(refs);result=-10;break;
   }
   textures_active=g->scene.texture_count!=0;
   loaded_scene=g->scene_index;
   int upload_error=numrefs &&
       cnk_vitagl_scene_upload_verified(&track_textures,&g->scene,
                                         g->scene_index,refs,numrefs);
   free(refs);
   if(upload_error){result=-11;break;}
  }
  glViewport(0,0,960,544);
  glClearColor(0.04f,0.07f,0.12f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  int draw_rc=0;
  if(have_kart_gallery && snapshot.state==CNK_FRONTEND_SELECT){
   unsigned kart_calls=0;
   cnk_gallery_material_binding material[CNK_SOURCE_KART_FAMILIES]={{0}};
   if((gallery_skins && cnk_gallery_skins_bind(gallery_skins,gallery_lod,material))||
      cnk_source_kart_gallery_draw_mapped_at(&kart_gallery,gallery_lod,
                            (float)gallery_angle_step*0.5235987756f,
                            gallery_skins?material:NULL,&kart_calls)){
    result=-40;break;
   }
  }else if(have_gallery && snapshot.state==CNK_FRONTEND_SELECT){
   unsigned gallery_calls=0;
   if(cnk_character_gallery_draw_at(&character_pack,
           (float)gallery_angle_step*0.5235987756f,&gallery_calls)){
    result=-37;break;
   }
   /* Models/textures are authentic; preview pose and gallery are not. */
  }else if(app.frontend.game.loaded){
   cnk_vitagl_frame_result frame={0};
   /* Per-frame distance LOD from explicitly verified 3DOB assignments.
    * A missing mapping means no invented vehicle model. */
   if(cnk_racer_assets_attachments_for_camera(&racer_assets,
            &app.frontend.game.session,visuals)){
     result=-23;break;
   }
   if(preview_active && cnk_dev_race_visuals_fill(&preview_karts,
          app.frontend.game.session.racer_count,visuals)){result=-46;break;}
   if(kart_skins && cnk_kart_skins_apply(kart_skins,visuals)){result=-30;break;}
   draw_rc=cnk_vitagl_draw_game_frame_with_items_culled(&app.frontend.game,
       textures_active?cnk_vitagl_textures_resolve:NULL,
       textures_active?&track_textures:NULL,visuals,
       have_drivers?drivers:NULL,
       item_textures_active?resolve_item_texture:NULL,
       item_textures_active?&item_textures:NULL,&frame);
  }
  /* A failed draw must not be mistaken for a successful race frame. In
   * particular, do not submit an incomplete display list to the GPU. */
  if(draw_rc){result=-47;break;}
  debug_overlay(&snapshot,draw_rc,have_gallery,have_kart_gallery,
                gallery_lod,&native_lang,preview_active);
  /* GL errors (for example, exhausted video memory) do not necessarily
   * change a draw function's return value. Diagnose them before swapping;
   * cleanup below still owns and releases the scene and texture handles. */
  if(glGetError()!=GL_NO_ERROR){result=-48;break;}
  /* Vita panel refresh is 60 Hz. Pace rendering to vblank instead of
   * busy-rendering unlimited frames (tearing, needless CPU/GPU load).
   * Simulation stays governed by the monotonic time delta above.
   * This does not pretend to reproduce Zeebo frame pacing or physics. */
  vglSwapBuffers(GL_FALSE);
  if(sceDisplayWaitVblankStart()<0){result=-13;break;}
  /* Record only successfully presented frames, never aborted GL draws. */
  cnk_vita_telemetry_present(&perf,sceKernelGetProcessTimeWide());
 }
cleanup:
 /* Join playback first. Only THEN may cue PCM backing be freed. */
 if(audio_initialized){
  cnk_vita_audio_worker_close(&audio); /* joins before sample release */
  cnk_music_cues_close(&music);
  cnk_sound_cues_close(&cues);
 }
 if(gpu_started){
  cnk_vitagl_textures_free(&track_textures);
  cnk_vitagl_textures_free(&item_textures);
  cnk_character_pack_free(&character_pack);
  if(gallery_skins)cnk_gallery_skins_free(gallery_skins);
  if(kart_skins)cnk_kart_skins_free(kart_skins);
  /* vitaGL does not expose a public vglEnd() teardown API. All\n   * application-owned GPU resources were released above; the runtime\n   * releases its remaining internals when the process terminates. */
 }
 free(gallery_skins);
 free(kart_skins);
 cnk_source_kart_gallery_free(&kart_gallery);
 cnk_dev_race_visuals_free(&preview_karts);
 cnk_racer_assets_free(&racer_assets);
 cnk_png_catalog_free(&png);
 cnk_texture_manifest_free(&manifest);
 cnk_app_runtime_close(&app);
 cnk_lang_free(&native_lang);
 fufs_close(&vfs);
 /* Best-effort breadcrumb; never override the original failure code. A
  * normal exit overwrites an old error so stale logs are not misleading. */
 (void)cnk_vita_telemetry_write("ux0:data/cnk3d_perf.txt",&perf,result);
 (void)cnk_vita_diagnostic_write("ux0:data/cnk3d_last_status.txt",result);
 if(result)fprintf(stderr,"CNK Vita: %d %s\n",result,
                    cnk_vita_diagnostic_name(result));
 return result;
}

int cnk_vita_native_run_with_driver_assets(const char *vfs_path,
                const char *manifest_path,const char *racer_map_path,
                const char *sound_map_path,const char *music_map_path,
                const char *music_directory,const char *driver_map_path){
 return cnk_vita_native_run_with_kart_skins(vfs_path,manifest_path,racer_map_path,
    sound_map_path,music_map_path,music_directory,driver_map_path,NULL);
}

int cnk_vita_native_run_with_music(const char *vfs_path,const char *manifest_path,
                const char *racer_map_path,const char *sound_map_path,
                const char *music_map_path,const char *music_directory){
 return cnk_vita_native_run_with_driver_assets(vfs_path,manifest_path,
        racer_map_path,sound_map_path,music_map_path,music_directory,NULL);
}

int cnk_vita_native_run_with_resources(const char *vfs_path,const char *manifest_path,
                                    const char *racer_map_path,const char *sound_map_path){
 return cnk_vita_native_run_with_music(vfs_path,manifest_path,
                                      racer_map_path,sound_map_path,NULL,NULL);
}
int cnk_vita_native_run_with_map(const char *vfs_path,const char *manifest_path){
 return cnk_vita_native_run_with_resources(vfs_path,manifest_path,NULL,NULL);
}
int cnk_vita_native_run(const char *vfs_path){
 /* Deliberately NOT supplied with guessed texture IDs. Only an explicit,
  * independently reviewed sidecar can assign images to material names. */
 return cnk_vita_native_run_with_kart_skins(vfs_path,
         "app0:/verified_texture_map.tsv","app0:/verified_racer_models.tsv",
         "app0:/verified_sound_cues.tsv","app0:/verified_music_cues.tsv",
         "app0:/music","app0:/verified_drivers.tsv",
         "app0:/verified_kart_textures.tsv");
}

int cnk_vita_native_run_with_assets(const char *vfs_path,
                                    const char *manifest_path,
                                    const char *racer_map_path){
 return cnk_vita_native_run_with_resources(vfs_path,manifest_path,racer_map_path,NULL);
}
