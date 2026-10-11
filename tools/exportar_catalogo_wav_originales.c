/* Read-only, reproducible inventory of WAV objects actually present in
 * Zeebo data.vfs. No event, actor or track identities are inferred. */
#include "fufs.h"
#include "cnk_audio.h"
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"Usage: %s data.vfs\n",argv[0]);return 2;}
 fufs_file vfs={0};
 if(fufs_open(argv[1],&vfs)){fputs("Cannot read original archive\n",stderr);return 3;}
 /* Produce no partial output if even one purported WAV has corrupt PCM. */
 FILE *spool=tmpfile();
 if(!spool){fufs_close(&vfs);return 4;}
 unsigned count=0;
 for(uint32_t i=0;i<vfs.count;i++){
  uint8_t *bytes=NULL;size_t n=0;
  if(fufs_extract(&vfs,i,&bytes,&n)){fclose(spool);fufs_close(&vfs);return 5;}
  if(!strcmp(fufs_type(bytes,n),"wav")){
   cnk_wav sound={0};
   if(cnk_wav_open(bytes,n,&sound)){
    fprintf(stderr,"WAV resource invalid at index %u\n",i);
    free(bytes);fclose(spool);fufs_close(&vfs);return 6;
   }
   /* The actual decoded PCM bytes, not the compressed FUFS payload. */
   uLong hash=crc32(0,Z_NULL,0);
   hash=crc32(hash,sound.pcm,(uInt)sound.pcm_bytes);
   if(fprintf(spool,"%u\t%08x\t%u\t%u\t%u\t%u\t%llu\t%08lx\n",
      i,vfs.entries[i].id,sound.sample_rate,(unsigned)sound.channels,
      (unsigned)sound.bits_per_sample,sound.frame_count,
      (unsigned long long)((uint64_t)sound.frame_count*1000u/sound.sample_rate),
      (unsigned long)hash)<0){free(bytes);fclose(spool);fufs_close(&vfs);return 7;}
   count++;
  }
  free(bytes);
 }
 rewind(spool);
 puts("# WAV assets extracted directly from original Zeebo FUFS archive, no cue bindings");
 puts("vfs_index\tfufs_id_hex\tsample_rate_hz\tchannels\tpcm_bits\tframes\tduration_ms\tpcm_crc32");
 int c;
 while((c=fgetc(spool))!=EOF)if(putchar(c)==EOF){fclose(spool);fufs_close(&vfs);return 8;}
 fclose(spool);fufs_close(&vfs);
 if(!count){fputs("No WAV samples found\n",stderr);return 9;}
 return 0;
}
