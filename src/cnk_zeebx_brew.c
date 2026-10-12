/* Structural parsing adapted from:
 *  ZeebxTeam/zeebx-emu/src/loader/modfile.rs
 *  ZeebxTeam/zeebx-emu/src/loader/miffile.rs
 * License GPL-2.0-or-later. This is a modified C99 port: validation,
 * output representation and corruption limits differ from Rust original.
 * The parser does NOT perform any ARM execution or BREW API dispatch.
 */
#include "cnk_zeebx_brew.h"
#include <string.h>
#include <stdint.h>

#define MAX_INPUT (32u*1024u*1024u)
static uint16_t u16(const uint8_t *p){
 return (uint16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8);
}
static uint32_t u32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
       (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int branch_target(uint32_t first,uint32_t pc,uint32_t *out){
 if((first&0xff000000u)!=0xea000000u)return -1;
 uint32_t bits=(first&0x00ffffffu);
 int64_t words=(bits&0x00800000u)?
  (int64_t)bits-(int64_t)0x01000000u:(int64_t)bits;
 int64_t target=(int64_t)pc+8+words*4;
 if(target<0||target>UINT32_MAX)return -2;
 *out=(uint32_t)target;
 return 0;
}
int cnk_brew_mod_inspect(const uint8_t *file,size_t size,cnk_brew_mod_info *out){
 cnk_brew_mod_info info={0};
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!file||size<8||size>MAX_INPUT)return -2;
 if(size>UINT32_MAX)return -2;
 info.first_instruction=u32(file);
 info.image_bytes=(uint32_t)size;
 uint32_t branch=0;
 if(!branch_target(info.first_instruction,0u,&branch)){
  if((branch&3u)==0u&&branch<size){
   info.has_initial_branch=1;
   info.first_branch_target=branch;
  }
 }
 if(size>=12&&memcmp(file+8,"BREW",4)==0){
  if(size<0x40)return -3;
  info.has_brew_header=1;
  info.brew_version=u32(file+12);
  info.code_offset=u32(file+16);
  if(info.code_offset<0x40u||info.code_offset>=size||
     (info.code_offset&3u)!=0u)return -4;
  for(unsigned i=0;i<10;i++)info.header_words[i]=u32(file+0x14u+i*4u);
  if(!info.has_initial_branch)return -5;
  info.entry_offset=info.first_branch_target;
  if(info.entry_offset<info.code_offset)return -6;
 }else{
  /* Mirror Zeebx: a bare .mod starts at 0, which can itself contain a
   * branch to the actual AEEMod_Load routine (Crash does exactly this). */
  if((info.first_instruction>>28)==15u)return -7;
  info.entry_offset=0;
 }
 memcpy(out,&info,sizeof(info));
 return 0;
}
int cnk_brew_mif_inspect(const uint8_t *file,size_t size,cnk_brew_mif_info *out){
 cnk_brew_mif_info info={0};
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!file||size<0x20||size>MAX_INPUT)return -2;
 if(u16(file)!=0x11u)return -3;
 /* Exactly as Zeebx's MifFile parser, the table at 0x10 contains
  * n+1 section end offsets. Do not mistake 0x08 for that table. */
 uint32_t table=u32(file+0x10);
 uint32_t count=u32(file+0x14);
 if(!count||count>CNK_BREW_MAX_SECTIONS||table<0x20u||
    (uint64_t)table+((uint64_t)count+1u)*4u>size)return -4;
 uint32_t before=0;
 for(uint32_t i=0;i<=count;i++){
  uint32_t off=u32(file+(size_t)table+(size_t)i*4u);
  if(off>size||(i&&off<before))return -5;
  if(i==0)info.first_section_offset=off;
  if(i==count)info.last_section_end=off;
  if(i!=count){
   uint32_t end=u32(file+(size_t)table+(size_t)(i+1u)*4u);
   if(end<off||end>size)return -5;
   uint32_t len=end-off;
   if(len==20u){
    /* Zeebx: only 20-byte records with two required zero words
     * (offset+4, offset+12) represent applet class IDs. */
    if(u32(file+off+4u)==0u&&u32(file+off+12u)==0u){
     uint32_t applet=u32(file+off);
     if(applet){
      if(info.applet_count==0u)info.main_applet=applet;
      info.applet_count++;
     }
    }
   }else if(len==8u){
    uint32_t cls=u32(file+off);
    if(cls && !u32(file+off+4u)){
     if(info.class_count>=CNK_BREW_MAX_CLASSES)return -6;
     info.classes[info.class_count++]=cls;
    }
   }
  }
  before=off;
 }
 info.sections=count;
 memcpy(out,&info,sizeof(info));
 return 0;
}
