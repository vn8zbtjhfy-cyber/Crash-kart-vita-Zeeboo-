#define _POSIX_C_SOURCE 200809L
#include "cnk_sound_cues.h"
#include "cnk_audio.h"
#include "fufs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* Real, unmodified ROM PCM with a TEST-ONLY event assignment. No claim
 * about the historical meaning of either Zeebo WAV file. */
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};
 assert(!fufs_open(argv[1],&vfs));
 assert(vfs.count>47&&vfs.entries[44].id==0x19df923du&&
        vfs.entries[47].id==0x1b130d8eu);
 cnk_audio_mixer m={0};
 assert(!cnk_audio_init(&m,48000));
 cnk_wav_asset external={0};
 assert(!cnk_wav_asset_load(&external,&vfs,44));
 char path[]="/tmp/cnk_audio_polyphony_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);close(fd);
 FILE *f=fopen(path,"wb");assert(f);
 const char *map="APP\tGO\t47\t1b130d8e\t0.5\n";
 assert(fwrite(map,1,strlen(map),f)==strlen(map));
 assert(!fclose(f));
 cnk_sound_cues router={0};
 assert(!cnk_sound_cues_read(&router,&m,&vfs,path));
 assert(!unlink(path));
 cnk_app_snapshot event={0};event.app_events=CNK_APP_EVENT_GO;
 unsigned played=999;
 /* Another sound subsystem owns the looping voice at slot 0. */
 assert(cnk_audio_play(&m,&external.wav,0.3f,1)==0);
 for(int i=1;i<(int)CNK_AUDIO_MAX_VOICES;i++){
  assert(cnk_audio_play(&m,&router.cues[0].asset.wav,0.3f,0)==i);
 }
 assert(cnk_audio_active(&m)==CNK_AUDIO_MAX_VOICES);
 /* A near-complete SFX should be replaced before less-progressed SFX. */
 const unsigned slot=7;
 m.voices[slot].frame_position=m.voices[slot].source.frame_count*0.95;
 assert(!cnk_sound_cues_dispatch(&router,&event,&played));
 assert(played==1&&cnk_audio_active(&m)==CNK_AUDIO_MAX_VOICES);
 assert(m.voices[0].active&&m.voices[0].loop&&
        m.voices[0].source.pcm==external.wav.pcm);
 assert(m.voices[slot].active&&m.voices[slot].frame_position==0.0);
 /* All voices belong to a different system: effect must drop, not crash. */
 for(unsigned i=1;i<CNK_AUDIO_MAX_VOICES;i++){
  assert(!cnk_audio_stop(&m,i));
  assert(cnk_audio_play(&m,&external.wav,0.1f,1)==(int)i);
 }
 assert(!cnk_sound_cues_dispatch(&router,&event,&played));
 assert(played==0&&cnk_audio_active(&m)==CNK_AUDIO_MAX_VOICES);
 cnk_sound_cues_close(&router); /* MUST NOT stop or free external voice */
 assert(cnk_audio_active(&m)==CNK_AUDIO_MAX_VOICES);
 for(unsigned i=0;i<CNK_AUDIO_MAX_VOICES;i++)assert(!cnk_audio_stop(&m,i));
 cnk_wav_asset_free(&external);
 fufs_close(&vfs);
 puts("PASS: 16-voice original-PCM overload, safe SFX reclamation, looped music protection, graceful drop");
 return 0;
}
