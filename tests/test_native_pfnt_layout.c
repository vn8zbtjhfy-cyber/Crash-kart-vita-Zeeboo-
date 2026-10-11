#include "fufs.h"
#include "cnk_pfnt.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc,char **argv){
 fufs_file ar={0};size_t found=0;
 if(argc!=2)return 2;
 assert(fufs_open(argv[1],&ar)==0);
 for(uint32_t i=0;i<ar.count;i++){
   uint8_t *data=NULL;size_t size=0;
   cnk_pfnt_view f={0};cnk_pfnt_positioned_glyph q[8];
   cnk_pfnt_glyph g0,g1;
   size_t count=123u;uint32_t width=42u,height=17u;
   assert(fufs_extract(&ar,i,&data,&size)==0);
   if(strcmp(fufs_type(data,size),"pfnt")!=0){free(data);continue;}
   assert(cnk_pfnt_open(data,size,&f)==0);
   assert(cnk_pfnt_glyph_for(&f,f.codepoint_first,&g0)==0);
   assert(cnk_pfnt_glyph_for(&f,f.codepoint_last,&g1)==0);
   /* Every PFNT supports newline layout, even when U+000A lacks a glyph. */
   assert(cnk_pfnt_layout_utf8(&f,"\n",1,q,8,&count,&width,&height)==0);
   assert(count==0&&width==0&&height==2u*f.line_height_px);
   memset(q,0x5a,sizeof(q));count=123u;width=42u;height=17u;
   assert(cnk_pfnt_layout_utf8(&f,"\xff",1,q,8,&count,&width,&height)==-4);
   assert(count==123u&&width==42u&&height==17u);
   assert(q[0].codepoint==0x5a5a5a5au);
   assert(cnk_pfnt_layout_utf8(&f,"\xf0\x9f\x98\x80",4,q,8,&count,&width,&height)==-3);
   assert(count==123u&&width==42u&&height==17u);
   assert(cnk_pfnt_layout_utf8(&f,"\n",1,q,0,&count,&width,&height)==0);
   assert(cnk_pfnt_layout_utf8(&f,"\n",1,q,0,NULL,&width,&height)<0);
   /* Build UTF-8 for two authentic endpoints; avoid relying on ASCII
    * for source font 31, whose first character is U+0001. */
   unsigned char encoded[10]={0};size_t n0=0,n1=0;
   uint16_t cp[2]={f.codepoint_first,f.codepoint_last};
   for(int j=0;j<2;j++){
     uint16_t c=cp[j];size_t t=(j==0?0:n0);
     if(c<128){encoded[t]=(uint8_t)c;if(j==0)n0=1;else n1=1;}
     else if(c<2048){encoded[t]=(uint8_t)(0xc0|(c>>6));encoded[t+1]=(uint8_t)(0x80|(c&63));if(j==0)n0=2;else n1=2;}
     else{encoded[t]=(uint8_t)(0xe0|(c>>12));encoded[t+1]=(uint8_t)(0x80|((c>>6)&63));encoded[t+2]=(uint8_t)(0x80|(c&63));if(j==0)n0=3;else n1=3;}
   }
   encoded[n0+n1]='\n';memcpy(encoded+n0+n1+1,encoded,n0);
   assert(cnk_pfnt_layout_utf8(&f,(char*)encoded,n0+n1+1+n0,q,8,&count,&width,&height)==0);
   assert(count==3u&&height==2u*f.line_height_px);
   assert(width==g0.advance_px+g1.advance_px);
   assert(q[0].codepoint==f.codepoint_first&&q[0].dest_x_px==0&&q[0].dest_y_px==0);
   assert(q[1].codepoint==f.codepoint_last&&q[1].dest_x_px==g0.advance_px&&q[1].dest_y_px==0);
   assert(q[2].codepoint==f.codepoint_first&&q[2].dest_x_px==0&&q[2].dest_y_px==f.line_height_px);
   assert(q[1].source_x_px==g1.atlas_x_px);
   /* Insufficient capacity must not write even the first glyph. */
   memset(q,0x5a,sizeof(q));count=123;width=42;height=17;
   assert(cnk_pfnt_layout_utf8(&f,(char*)encoded,n0+n1+1+n0,q,2,&count,&width,&height)==-6);
   assert(count==123u&&width==42u&&height==17u&&q[0].codepoint==0x5a5a5a5au);
   free(data);found++;
 }
 fufs_close(&ar);assert(found==5);
 puts("PFNT original glyph metrics drive multiline UTF-8 positioning across all 5 sources");
 return 0;
}
