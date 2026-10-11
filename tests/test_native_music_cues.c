#define _POSIX_C_SOURCE 200809L
#include "cnk_music_cues.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

static int locked, acquired, released, refuse;
static int take(void *ctx){
 (void)ctx;
 if(refuse)return -1;
 assert(!locked);locked=1;acquired++;return 0;
}
static int give(void *ctx){
 (void)ctx;assert(locked);locked=0;released++;return 0;
}
static void save(const char *path,const char *contents){
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(contents,1,strlen(contents),f)==strlen(contents));
 assert(fclose(f)==0);
}
int main(int argc,char **argv){
 assert(argc==2);
 char path[]="/tmp/cnk_music_cues_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);close(fd);
 cnk_audio_mixer mixer={0};assert(cnk_audio_init(&mixer,48000)==0);
 cnk_music_cues router={0};
 /* Wrong historical IDs, duplicates, unsupported states and paths must
  * fail ATOMICALLY before they touch the running audio mixer. */
 save(path,"SELECT\t*\t999\t0.5\t1\n");
 assert(cnk_music_cues_read(&router,&mixer,path,CNK_SOURCE_ROOT "/assets/music_wav")<0);
 assert(!router.initialized&&cnk_audio_active(&mixer)==0);
 save(path,"SELECT\t*\t104\t0.5\t1\nSELECT\t*\t122\t0.5\t1\n");
 assert(cnk_music_cues_read(&router,&mixer,path,CNK_SOURCE_ROOT "/assets/music_wav")<0);
 save(path,"RACE\t*\t104\t0.8\t1\nRACE\t0\t122\t0.6\t1\n"
          "SELECT\t*\t104\t0.4\t1\nRESULTS\t*\t393\t0.6\t0\n");
 assert(cnk_music_cues_read(&router,&mixer,path,CNK_SOURCE_ROOT "/assets/music_wav")==0);
 assert(router.count==4&&router.initialized);
 cnk_app_snapshot s={0};unsigned changed=99;
 s.state=CNK_FRONTEND_SELECT;s.selected_vfs_index=0;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(changed==1&&router.player.track==0&&cnk_audio_active(&mixer)==1);
 int16_t output[512*2];double energy=0;
 for(unsigned n=0;n<80;n++){
  assert(cnk_audio_render(&mixer,output,512)==0);
  for(unsigned i=0;i<1024;i++)energy+=abs(output[i]);
 }
 assert(energy>1000.0);
 unsigned transitions=(unsigned)acquired;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(changed==0&&acquired==(int)transitions);
 s.state=CNK_FRONTEND_RACE;s.selected_vfs_index=0;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(router.player.track==1 && changed==1); /* exact beats * */
 s.selected_vfs_index=8;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(router.player.track==0&&changed==1); /* wildcard fallback */
 assert(router.playing_volume==0.8f);
 s.state=CNK_FRONTEND_PAUSE;
 unsigned voice_before=(unsigned)router.player.voice;
 double cursor_before=mixer.voices[voice_before].frame_position;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(changed==1&&router.player.track==0&&cnk_audio_active(&mixer)==1);
 assert(mixer.voices[voice_before].paused);
 assert(!cnk_audio_render(&mixer,output,512));
 assert(mixer.voices[voice_before].frame_position==cursor_before);
 for(unsigned i=0;i<1024;i++)assert(output[i]==0);
 /* Resume the same race, preserving song cursor and sample identity. */
 s.state=CNK_FRONTEND_RACE;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(changed==1&&router.player.track==0&&cnk_audio_active(&mixer)==1);
 assert((unsigned)router.player.voice==voice_before);
 assert(mixer.voices[voice_before].frame_position==cursor_before);
 assert(!mixer.voices[voice_before].paused);
 assert(!cnk_audio_render(&mixer,output,512));
 assert(mixer.voices[voice_before].frame_position>cursor_before);
 /* On entering results, explicit map supersedes the previous race music. */
 s.state=CNK_FRONTEND_RESULTS;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(router.player.track==6&&changed==1);
 s.state=CNK_FRONTEND_RACE;s.selected_vfs_index=0;
 refuse=1;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)<0);
 assert(router.player.track==6&&cnk_audio_active(&mixer)==1);
 refuse=0;
 assert(cnk_music_cues_dispatch(&router,&s,take,give,NULL,&changed)==0);
 assert(router.player.track==1&&cnk_audio_active(&mixer)==1);
 assert(acquired==released&&!locked);
 cnk_music_cues_close(&router);
 assert(cnk_audio_active(&mixer)==0);
 unlink(path);
 puts("Verified Zeebo music selection, original PCM mixing, state transitions and audio mutex OK");
 return 0;
}
