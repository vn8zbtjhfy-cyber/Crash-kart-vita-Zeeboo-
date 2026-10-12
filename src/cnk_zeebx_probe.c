#include "cnk_zeebx_probe.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Byte-owning startup adapter for Zeebx's optional CPU module.
 * No game data is put in the executable. A protected source path is read
 * in memory and freed before the diagnostic log is written. */
int cnk_zeebx_probe_original(const char *mod_path,const char *report_path,
                             uint32_t budget,cnk_arm_outcome *result){
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome outcome={0};
 uint8_t *data=NULL;
 size_t size=0;
 int status=-1;
 if(!result)return -1;
 memset(result,0,sizeof(*result));
 if(!mod_path||!report_path||!budget||budget>10000000u)return -1;
 FILE *input=fopen(mod_path,"rb");
 if(!input)return -2;
 if(fseek(input,0,SEEK_END)||ftell(input)<8||
    ftell(input)>16*1024*1024L||fseek(input,0,SEEK_SET)){
  fclose(input);return -3;
 }
 long len=ftell(input);
 /* After rewind, ftell is zero; measure via SEEK_END before rewind. */
 if(fseek(input,0,SEEK_END)){fclose(input);return -3;}
 len=ftell(input);
 if(len<8||len>16*1024*1024L||fseek(input,0,SEEK_SET)){
  fclose(input);return -3;
 }
 size=(size_t)len;
 data=(uint8_t*)malloc(size);
 if(!data){fclose(input);return -4;}
 if(fread(data,1,size,input)!=size){
  fclose(input);free(data);return -5;
 }
 fclose(input);
 status=cnk_zeebx_arm_init(&cpu,data,size);
 free(data);
 if(status)return -6;
 status=cnk_zeebx_arm_run(&cpu,budget,&outcome);
 if(status){cnk_zeebx_arm_destroy(&cpu);return -7;}
 /* The outcome is the exact stopping point, not a guessed API result. */
 FILE *report=fopen(report_path,"wb");
 if(!report){cnk_zeebx_arm_destroy(&cpu);return -8;}
 const char *reason=outcome.reason==CNK_ARM_API?"BREW_API":
                     outcome.reason==CNK_ARM_RETURNED?"RETURNED":
                     outcome.reason==CNK_ARM_FAULT?"MEMORY_FAULT":
                     outcome.reason==CNK_ARM_UNSUPPORTED?"UNSUPPORTED_OPCODE":
                     "INSTRUCTION_BUDGET";
 int written=fprintf(report,
  "original_mod_size=%lu\nreason=%s\nsteps=%llu\npc=0x%08x\n"
  "api_interface=%u\napi_slot=%u\nopcode=0x%08x\naddress=0x%08x\n"
  "genuine_game_playable=NO\n",
  (unsigned long)size,reason,(unsigned long long)outcome.executed,
  outcome.pc,outcome.api_interface,outcome.api_slot,outcome.opcode,
  outcome.address);
 int close_result=fclose(report);
 *result=outcome;
 cnk_zeebx_arm_destroy(&cpu);
 return written<0||close_result?-9:0;
}
