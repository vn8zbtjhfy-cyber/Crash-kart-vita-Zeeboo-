#include "cnk_lang.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <zlib.h>

#define MAX_LANG_DATA (128u*1024u)
static uint32_t get32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t get16(const unsigned char *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
/* Encodes source Unicode, rejects unpaired UTF-16 surrogates. */
static int convert(const unsigned char *s,size_t n,char *dest,size_t cap,size_t *written){
 size_t o=0;
 if((n&1u)||!dest||!written)return -1;
 for(size_t pos=0;pos<n;pos+=2){
  uint32_t cp=get16(s+pos);
  if(cp>=0xd800u&&cp<=0xdbffu){
   if(pos+4>n)return -2;
   uint32_t lo=get16(s+pos+2);
   if(lo<0xdc00u||lo>0xdfffu)return -2;
   cp=0x10000u+((cp-0xd800u)<<10)+(lo-0xdc00u);pos+=2;
  }else if(cp>=0xdc00u&&cp<=0xdfffu)return -2;
  size_t extra=cp<0x80u?1u:cp<0x800u?2u:cp<0x10000u?3u:4u;
  if(o>cap||extra>cap-o)return -3;
  if(extra==1u)dest[o++]=(char)cp;
  else if(extra==2u){dest[o++]=(char)(0xc0u|(cp>>6));dest[o++]=(char)(0x80u|(cp&63u));}
  else if(extra==3u){dest[o++]=(char)(0xe0u|(cp>>12));dest[o++]=(char)(0x80u|((cp>>6)&63u));dest[o++]=(char)(0x80u|(cp&63u));}
  else {dest[o++]=(char)(0xf0u|(cp>>18));dest[o++]=(char)(0x80u|((cp>>12)&63u));dest[o++]=(char)(0x80u|((cp>>6)&63u));dest[o++]=(char)(0x80u|(cp&63u));}
 }
 *written=o;return 0;
}
int cnk_lang_preference_read(const char *path,cnk_lang_locale *out){
 if(!path||!*path||!out)return -1;
 FILE *fp=fopen(path,"rb");
 if(!fp)return errno==ENOENT?-2:-3;
 char buf[8]={0};size_t n=fread(buf,1,sizeof(buf),fp);
 int io=ferror(fp);int rc=-4;
 if(!io && n>=1 && n<=3 && buf[0]>='0' && buf[0]<='4'){
  if(n==1||(n==2&&buf[1]=='\n')||
     (n==3&&buf[1]=='\r'&&buf[2]=='\n')){
   *out=(cnk_lang_locale)(buf[0]-'0');rc=0;
  }
 }
 fclose(fp);return rc;
}
int cnk_lang_preference_write(const char *path,cnk_lang_locale locale){
 if(!path||!*path||(unsigned)locale>=CNK_LANG_LOCALES)return -1;
 size_t len=strlen(path);
 if(len>4096u)return -1;
 char *tmp=(char*)malloc(len+5u);
 if(!tmp)return -3;
 memcpy(tmp,path,len);
 memcpy(tmp+len,".tmp",5);
 FILE *fp=fopen(tmp,"wb");
 if(!fp){free(tmp);return -3;}
 unsigned char option[2]={(unsigned char)('0'+locale),'\n'};
 int ok=fwrite(option,1,sizeof(option),fp)==sizeof(option);
 if(fflush(fp)!=0)ok=0;
 if(fclose(fp)!=0)ok=0;
 if(!ok){remove(tmp);free(tmp);return -3;}
 if(rename(tmp,path)!=0){remove(tmp);free(tmp);return -3;}
 free(tmp);return 0;
}
int cnk_lang_cycle(const fufs_file *f,cnk_lang_pack *current){
 if(!f||!current)return -1;
 uint32_t next=current->storage && current->locale<CNK_LANG_LOCALES ?
   (current->locale+1u)%CNK_LANG_LOCALES : CNK_LANG_SPANISH;
 return cnk_lang_load(f,(cnk_lang_locale)next,current);
}
void cnk_lang_free(cnk_lang_pack *lang){if(lang){free(lang->storage);memset(lang,0,sizeof(*lang));}}
const char *cnk_lang_text(const cnk_lang_pack *lang,uint32_t ordinal){
 return (lang&&lang->storage&&ordinal<lang->count)?lang->text[ordinal]:NULL;
}
int cnk_lang_load(const fufs_file *f,cnk_lang_locale locale,cnk_lang_pack *out){
 if(!f||!out||(unsigned)locale>=CNK_LANG_LOCALES||
    f->count<=CNK_LANG_SOURCE_INDEX||
    f->entries[CNK_LANG_SOURCE_INDEX].id!=CNK_LANG_SOURCE_ID)return -1;
 unsigned char *data=NULL,*raw=NULL;
 size_t n=0;cnk_lang_pack stage={0};int rc=-2;
 if(fufs_extract(f,CNK_LANG_SOURCE_INDEX,&data,&n))return -2;
 /* LANG: header (number of languages, number of strings, version, name
  * block length), UTF-16 names, 5 x 16-byte stream descriptors, zlib data. */
 if(n<32||n>MAX_LANG_DATA||memcmp(data,"LANG",4)||
    get32(data+4)!=CNK_LANG_LOCALES||get32(data+8)!=CNK_LANG_STRINGS||
    get32(data+12)!=2u)goto done;
 uint32_t names_size=get32(data+16);
 if(names_size>n-20u||names_size<2u)goto done;
 size_t table=20u+(size_t)names_size;
 if(table>n||CNK_LANG_LOCALES*16u>n-table)goto done;
 /* Validate EVERY descriptor so corruption in any language is caught. */
 size_t previous=table+CNK_LANG_LOCALES*16u;
 for(unsigned i=0;i<CNK_LANG_LOCALES;i++){
  const unsigned char *entry=data+table+i*16u;
  size_t offset=get32(entry),size=get32(entry+4),compressed=get32(entry+8),name_offset=get32(entry+12);
  if(offset!=previous||!compressed||size<2||size>MAX_LANG_DATA||
     compressed>n-offset||offset>n||name_offset>=names_size||
     (name_offset&1u))goto done;
  /* Name must be NUL-terminated in the source UTF-16 name block. */
  int terminated=0;
  for(size_t j=name_offset;j+1<names_size;j+=2)
   if(get16(data+20+j)==0){terminated=1;break;}
  if(!terminated)goto done;
  previous=offset+compressed;
 }
 if(previous!=n)goto done;
 const unsigned char *desc=data+table+(unsigned)locale*16u;
 size_t offset=get32(desc),unpacked=get32(desc+4),compressed=get32(desc+8);
 size_t name_at=20u+get32(desc+12),name_end=name_at;
 while(name_end+1<20u+names_size && get16(data+name_end)!=0)name_end+=2;
 if(name_end+1>=20u+names_size)goto done;
 size_t name_bytes=0;
 if(convert(data+name_at,name_end-name_at,stage.name,sizeof(stage.name)-1,&name_bytes))goto done;
 stage.name[name_bytes]=0;
 raw=(unsigned char*)malloc(unpacked);
 if(!raw){rc=-3;goto done;}
 uLongf got=(uLongf)unpacked;
 if(uncompress(raw,&got,data+offset,(uLong)compressed)!=Z_OK||got!=unpacked||(got&1u))goto done;
 /* At most three UTF-8 bytes per source UTF-16 code unit, plus NULs. */
 size_t cap=3u*(unpacked/2u)+1u,used=0,at=0;
 stage.storage=(char*)malloc(cap);
 if(!stage.storage){rc=-3;goto done;}
 for(unsigned i=0;i<CNK_LANG_STRINGS;i++){
  size_t start=at;
  while(at+1<unpacked&&get16(raw+at)!=0)at+=2;
  if(at+1>=unpacked){rc=-4;goto done;}
  stage.text[i]=stage.storage+used;
  size_t produced=0;
  if(convert(raw+start,at-start,stage.storage+used,cap-used,&produced)||
     produced>=cap-used){rc=-4;goto done;}
  used+=produced;stage.storage[used++]=0;at+=2;
 }
 if(at!=unpacked){rc=-4;goto done;}
 stage.locale=(uint32_t)locale;stage.count=CNK_LANG_STRINGS;
 /* Atomic commit: invalid input does not destroy an existing loaded pack.
  * On successful reload, release the previous pack instead of leaking it. */
 char *old_storage=out->storage;
 *out=stage;memset(&stage,0,sizeof(stage));free(old_storage);rc=0;
 done:free(raw);free(data);cnk_lang_free(&stage);return rc;
}
