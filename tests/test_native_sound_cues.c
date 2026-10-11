#define _POSIX_C_SOURCE 200809L
#include "cnk_sound_cues.h"
#include "cnk_race.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void put(const char *path,const char *source){
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(source,1,strlen(source),f)==strlen(source));
 assert(!fclose(f));
}
static unsigned energy(const int16_t *samples,unsigned count){
 unsigned sum=0;
 for(unsigned i=0;i<count;i++)if(samples[i])sum++;
 return sum;
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 assert(vfs.count>104 && vfs.entries[44].id==0x19df923du &&
        vfs.entries[47].id==0x1b130d8eu);
 cnk_audio_mixer m={0};assert(!cnk_audio_init(&m,48000));
 char path[]="/tmp/cnk_sound_cues_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);assert(!close(fd));
 cnk_sound_cues c={0};unsigned played=999;
 /* NO historical assignment: explicitly synthetic mapping for this test. */
 put(path,"# TEST ONLY: these are not source-verified event associations\n"
   "APP\tRACE_STARTED\t44\t19df923d\t0.5\n"
   "PLAYER\tLAP\t47\t1b130d8e\t0.7\n");
 assert(!cnk_sound_cues_read(&c,&m,&vfs,path));
 assert(c.count==2);
 cnk_app_runtime app={0};cnk_app_snapshot snapshot={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 assert(!cnk_app_runtime_tick(&app,1000000,0,(cnk_driver_input){0},&snapshot));
 assert(!cnk_sound_cues_dispatch(&c,&snapshot,&played)&&played==0);
 assert(!cnk_app_runtime_tick(&app,1016667,CNK_UI_CONFIRM,
          (cnk_driver_input){0},&snapshot));
 assert(snapshot.app_events&CNK_APP_EVENT_RACE_STARTED);
 assert(!cnk_sound_cues_dispatch(&c,&snapshot,&played)&&played==1);
 assert(cnk_audio_active(&m)==1);
 app.frontend.game.last_race_events[0]=CNK_RACE_LAP;
 assert(!cnk_app_runtime_tick(&app,1016667,0,(cnk_driver_input){0},&snapshot));
 assert(snapshot.race_events[0]&CNK_RACE_LAP);
 assert(!cnk_sound_cues_dispatch(&c,&snapshot,&played)&&played==1);
 assert(cnk_audio_active(&m)==2);
 int16_t output[1024]={0};
 assert(!cnk_audio_render(&m,output,512));
 assert(energy(output,1024)>0);
 /* Reload attempts MUST be atomic: a wrong source ID cannot steal voices. */
 put(path,"APP\tRACE_STARTED\t44\t00000000\t0.5\n");
 assert(cnk_sound_cues_read(&c,&m,&vfs,path)<0);
 assert(c.count==2 && cnk_audio_active(&m)==2);
 put(path,"APP\tRACE_STARTED\t44\t19df923d\t0.5\n"
          "APP\tRACE_STARTED\t47\t1b130d8e\t0.7\n");
 assert(cnk_sound_cues_read(&c,&m,&vfs,path)<0);
 put(path,"APP\tRACE_STARTED\t104\t3a77b37b\t0.5\n"); /* MP3, not WAV */
 assert(cnk_sound_cues_read(&c,&m,&vfs,path)<0);
 put(path,"APP\tRACE_STARTED\t44\t19df923d\t0\n");
 assert(cnk_sound_cues_read(&c,&m,&vfs,path)<0);
 put(path,"APP\tRACE_STARTED\t44\t19df923d\t0.5\textra\n");
 assert(cnk_sound_cues_read(&c,&m,&vfs,path)<0);
 assert(c.count==2 && cnk_audio_active(&m)==2);
 /* New valid assignments stop active voices BEFORE owned PCM is freed. */
 put(path,"APP\tPAUSED\t47\t1b130d8e\t0.4\n");
 assert(!cnk_sound_cues_read(&c,&m,&vfs,path));
 assert(c.count==1 && cnk_audio_active(&m)==0);
 cnk_app_snapshot test={0};test.app_events=CNK_APP_EVENT_PAUSED;
 assert(!cnk_sound_cues_dispatch(&c,&test,&played)&&played==1);
 assert(cnk_audio_active(&m)==1);
 /* Test-only assignments prove new runtime events are addressable; no
  * historical event/WAV identity is inferred from these valid ROM bytes. */
 put(path,"APP\tINCOMING_PROJECTILE\t44\t19df923d\t0.4\n"
          "APP\tKART_RECOVERED\t47\t1b130d8e\t0.4\n");
 assert(!cnk_sound_cues_read(&c,&m,&vfs,path)&&c.count==2);
 test.app_events=CNK_APP_EVENT_INCOMING_PROJECTILE|CNK_APP_EVENT_KART_RECOVERED;
 assert(!cnk_sound_cues_dispatch(&c,&test,&played)&&played==2);
 cnk_sound_cues_close(&c);
 assert(cnk_audio_active(&m)==0 && c.count==0);
 assert(!unlink(path));
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 puts("PASS: original PCM resources routed by explicit, validated event associations; no assumed Zeebo sound mapping; atomic reload and voice ownership verified");
 return 0;
}
