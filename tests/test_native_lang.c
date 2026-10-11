#include "cnk_lang.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *source_names[5]={"ENGLISH","FRANÇAIS","ITALIANO","PORTUGUÊS","ESPAÑOL"};
static const char *race_names[5]={"QUICK RACE","COURSE RAPIDE","CORSA RAPIDA","CORRIDA RÁPIDA","CARRERA RÁPIDA"};
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file source={0};assert(fufs_open(argv[1],&source)==0);
 assert(source.entries[CNK_LANG_SOURCE_INDEX].id==CNK_LANG_SOURCE_ID);
 cnk_lang_pack lang={0};
 for(int i=0;i<5;i++){
  assert(cnk_lang_load(&source,(cnk_lang_locale)i,&lang)==0);
  assert(lang.count==229&&lang.locale==(uint32_t)i);
  assert(strcmp(lang.name,source_names[i])==0);
  assert(strcmp(cnk_lang_text(&lang,0),"OK")==0);
  assert(strcmp(cnk_lang_text(&lang,14),race_names[i])==0);
  assert(cnk_lang_text(&lang,CNK_LANG_STRINGS)==NULL);
  assert(cnk_lang_text(&lang,228)!=NULL);
  printf("Original LANG %d (%s): 229 source strings; sample %s\n",i,lang.name,cnk_lang_text(&lang,14));
  cnk_lang_free(&lang);
 }
 /* Optional locale selection rejects malformed preferences atomically. */
 cnk_lang_locale preference=CNK_LANG_SPANISH;
 const char *cfg="cnk_test_language_option.tmp";
 FILE *cf=fopen(cfg,"wb");assert(cf);assert(fwrite("1\n",1,2,cf)==2);fclose(cf);
 assert(cnk_lang_preference_read(cfg,&preference)==0&&preference==CNK_LANG_FRENCH);
 cf=fopen(cfg,"wb");assert(cf);assert(fwrite("9\n",1,2,cf)==2);fclose(cf);
 assert(cnk_lang_preference_read(cfg,&preference)<0&&preference==CNK_LANG_FRENCH);
 assert(remove(cfg)==0);
 assert(cnk_lang_preference_read(cfg,&preference)==-2);
 /* A user-writable choice can replace a prior one atomically. */
 assert(cnk_lang_preference_write(cfg,CNK_LANG_ITALIAN)==0);
 preference=CNK_LANG_ENGLISH;
 assert(cnk_lang_preference_read(cfg,&preference)==0 && preference==CNK_LANG_ITALIAN);
 assert(cnk_lang_preference_write(cfg,(cnk_lang_locale)8)<0);
 assert(cnk_lang_preference_read(cfg,&preference)==0 && preference==CNK_LANG_ITALIAN);
 assert(cnk_lang_preference_write("",CNK_LANG_SPANISH)<0);
 assert(remove(cfg)==0);
 assert(cnk_lang_load(&source,CNK_LANG_SPANISH,&lang)==0);
 assert(strcmp(cnk_lang_text(&lang,25),"CONTRARRELOJ")==0);
 assert(strcmp(cnk_lang_text(&lang,40),"¡NUEVO RÉCORD!")==0);
 assert(strcmp(cnk_lang_text(&lang,29),"ELEGIR PISTA")==0);
 /* Repeat successful replacements on the same pack, then test rollback. */
 for(int loop=0;loop<15;loop++){
  assert(cnk_lang_cycle(&source,&lang)==0);
  assert(lang.locale==(uint32_t)((CNK_LANG_SPANISH+loop+1)%CNK_LANG_LOCALES));
  assert(!strcmp(cnk_lang_text(&lang,0),"OK"));
 }
 assert(cnk_lang_load(&source,CNK_LANG_SPANISH,&lang)==0);
 assert(strcmp(cnk_lang_text(&lang,29),"ELEGIR PISTA")==0);
 /* Invalid lookups do not mutate existing valid content. */
 assert(cnk_lang_load(&source,(cnk_lang_locale)6,&lang)<0);
 assert(strcmp(cnk_lang_text(&lang,29),"ELEGIR PISTA")==0);
 fufs_file tampered=source;fufs_entry *entries=(fufs_entry*)malloc(source.count*sizeof(*entries));
 assert(entries);memcpy(entries,source.entries,source.count*sizeof(*entries));
 tampered.entries=entries;
 entries[CNK_LANG_SOURCE_INDEX].id^=1u;
 assert(cnk_lang_load(&tampered,CNK_LANG_ENGLISH,&lang)<0);
 assert(strcmp(cnk_lang_text(&lang,29),"ELEGIR PISTA")==0);
 entries[CNK_LANG_SOURCE_INDEX].id=CNK_LANG_SOURCE_ID;
 unsigned char *copy=(unsigned char*)malloc(source.length);assert(copy);
 memcpy(copy,source.data,source.length);tampered.data=copy;
 /* Corrupt source language descriptor without touching user data.vfs. */
 size_t offset=entries[CNK_LANG_SOURCE_INDEX].offset;
 assert(offset+108+16*5<source.length);
 copy[offset+108+8]=0xff;copy[offset+108+9]=0xff;
 assert(cnk_lang_load(&tampered,CNK_LANG_ENGLISH,&lang)<0);
 assert(strcmp(cnk_lang_text(&lang,29),"ELEGIR PISTA")==0);
 free(copy);free(entries);
 cnk_lang_free(&lang);fufs_close(&source);
 puts("LANG PASS: all 1,145 texts source-verified; UTF16->UTF8; rollback on invalid data");
 return 0;
}
