/* Original-code execution probe. GPL-2.0-or-later: this command is
 * linked with the Zeebx-adapted interpreter, NOT a standalone emulator.
 * It reports the first unsupported opcode or BREW API without pretending
 * any game graphics or gameplay were successfully reproduced.
 * Runs only on user's machine, never uploads original cnk2.mod.
 */
#include "cnk_zeebx_arm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static uint8_t *read_file(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
 long n=ftell(f);
 if(n<=0||n>16*1024*1024L||fseek(f,0,SEEK_SET)){
  fclose(f);return NULL;
 }
 uint8_t *p=(uint8_t*)malloc((size_t)n);
 if(!p){fclose(f);return NULL;}
 size_t r=fread(p,1,(size_t)n,f);
 fclose(f);
 if(r!=(size_t)n){free(p);return NULL;}
 *size=(size_t)n;return p;
}
int main(int argc,char **argv){
 if(argc!=2){
  fprintf(stderr,"uso: %s /ruta/propia/cnk2.mod\n",argv[0]);
  return 2;
 }
 size_t length=0;
 uint8_t *bytes=read_file(argv[1],&length);
 cnk_zeebx_arm cpu={0};
 if(!bytes){fprintf(stderr,"mod original inexistente o invalido\n");return 2;}
 int init=cnk_zeebx_arm_init(&cpu,bytes,length);
 free(bytes);
 if(init){fprintf(stderr,"No se pudo preparar guest: %d\n",init);return 3;}
 cnk_arm_outcome outcome={0};
 int rc=cnk_zeebx_arm_run(&cpu,200000u,&outcome);
 if(rc){fprintf(stderr,"Ejecucion ARM fallo: %d\n",rc);}
 else{
  const char *reason=outcome.reason==CNK_ARM_API?"BREW API":
                     outcome.reason==CNK_ARM_RETURNED?"RETORNO":
                     outcome.reason==CNK_ARM_FAULT?"MEMORIA NO MAPEADA":
                     outcome.reason==CNK_ARM_UNSUPPORTED?"OPCODE PENDIENTE":
                     "PRESUPUESTO";
  printf("Zeebx ARM/BREW: %s; pasos=%llu pc=0x%08x",
         reason,(unsigned long long)outcome.executed,outcome.pc);
  if(outcome.reason==CNK_ARM_API)
   printf(" interfaz=%u slot=%u",outcome.api_interface,outcome.api_slot);
  if(outcome.reason==CNK_ARM_UNSUPPORTED)
   printf(" opcode=0x%08x",outcome.opcode);
  if(outcome.reason==CNK_ARM_FAULT)
   printf(" direccion=0x%08x",outcome.address);
  putchar('\n');
  if(outcome.reason!=CNK_ARM_API&&outcome.reason!=CNK_ARM_RETURNED)
   puts("No es un arranque completo: aqui falta compatibilidad ARM/BREW.");
 }
 cnk_zeebx_arm_destroy(&cpu);
 return rc?1:0;
}
