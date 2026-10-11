#include "fufs.h"
#include "cnk_pfnt.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void encode_utf8(uint16_t cp,char out[4],size_t *n){
 if(cp<=0x7fu){out[0]=(char)cp;*n=1;}
 else if(cp<=0x7ffu){out[0]=(char)(0xc0|(cp>>6));out[1]=(char)(0x80|(cp&63));*n=2;}
 else{out[0]=(char)(0xe0|(cp>>12));out[1]=(char)(0x80|((cp>>6)&63));out[2]=(char)(0x80|(cp&63));*n=3;}
}
static void exercise_source(const unsigned char *blob,size_t len){
 cnk_pfnt_view f={0},rejected={0};cnk_pfnt_glyph a={0},b={0};
 uint8_t *corrupt=(uint8_t*)malloc(len);uint32_t width=1234;
 uint32_t running=0;char chars[8];size_t used1,used2;
 assert(corrupt);
 assert(cnk_pfnt_open(blob,len,&f)==0);
 assert(f.glyph_count>0&&f.trailing_payload_offset<len);
 for(uint32_t cp=f.codepoint_first;cp<=f.codepoint_last;cp++){
   int ret=cnk_pfnt_glyph_for(&f,cp,&a);
   if(ret==1)continue;
   assert(ret==0&&a.codepoint==cp&&a.atlas_x_px==running);
   running+=a.advance_px;
   encode_utf8((uint16_t)cp,chars,&used1);
   width=0;
   assert(cnk_pfnt_measure_utf8(&f,chars,used1,&width)==0&&width==a.advance_px);
 }
 assert(running==f.total_advance_px);
 assert(cnk_pfnt_glyph_for(&f,f.codepoint_first,&a)==0);
 assert(cnk_pfnt_glyph_for(&f,f.codepoint_last,&b)==0);
 encode_utf8(f.codepoint_first,chars,&used1);
 encode_utf8(f.codepoint_last,chars+used1,&used2);
 width=0;
 assert(cnk_pfnt_measure_utf8(&f,chars,used1+used2,&width)==0);
 assert(width==(uint32_t)a.advance_px+b.advance_px);
 width=999;
 assert(cnk_pfnt_measure_utf8(&f,"",0,&width)==0&&width==0);
 assert(cnk_pfnt_glyph_for(&f,0x1f600u,&a)==1);
 width=999;
 assert(cnk_pfnt_measure_utf8(&f,"\xf0\x9f\x98\x80",4,&width)==-3&&width==999);
 assert(cnk_pfnt_measure_utf8(&f,"\xc0\x80",2,&width)==-4&&width==999);
 assert(cnk_pfnt_measure_utf8(&f,"\xed\xa0\x80",3,&width)==-4&&width==999);
 assert(cnk_pfnt_measure_utf8(&f,"\xe2\x82",2,&width)==-4&&width==999);
 assert(cnk_pfnt_open(blob,f.trailing_payload_offset-1,&rejected)!=0);
 assert(!rejected.bytes);
 memcpy(corrupt,blob,len);corrupt[0]='B';
 assert(cnk_pfnt_open(corrupt,len,&rejected)!=0&&!rejected.bytes);
 memcpy(corrupt,blob,len);corrupt[18+2*f.glyph_count+3]=0; /* width zero */
 assert(cnk_pfnt_open(corrupt,len,&rejected)!=0&&!rejected.bytes);
 memcpy(corrupt,blob,len);corrupt[18+2*f.glyph_count+4]=0xff;/* breaks offsets */
 assert(cnk_pfnt_open(corrupt,len,&rejected)!=0&&!rejected.bytes);
 memcpy(corrupt,blob,len);corrupt[16]=0xff;corrupt[17]=0xff;
 assert(cnk_pfnt_open(corrupt,len,&rejected)!=0&&!rejected.bytes);
 memcpy(corrupt,blob,len);corrupt[18+2*f.glyph_count]=1; /* invalid first atlas x */
 assert(cnk_pfnt_open(corrupt,len,&rejected)!=0&&!rejected.bytes);
 free(corrupt);
}
int main(int argc,char **argv){
 fufs_file archive={0};size_t found=0;
 if(argc!=2)return 2;
 assert(fufs_open(argv[1],&archive)==0);
 for(uint32_t i=0;i<archive.count;i++){
   unsigned char *p=NULL;size_t n=0;
   assert(fufs_extract(&archive,i,&p,&n)==0);
   if(strcmp(fufs_type(p,n),"pfnt")==0){
     exercise_source(p,n);found++;
   }
   free(p);
 }
 fufs_close(&archive);
 assert(found==5);
 puts("Verified 5 authentic PFNT index tables, advances, UTF-8 widths, corrupt/truncated rejection");
 return 0;
}
