#include "cnk_zeebx_brew.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void wr16(unsigned char *p,uint16_t x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);}
static void wr32(unsigned char *p,uint32_t x){for(int i=0;i<4;i++)p[i]=(unsigned char)(x>>(8*i));}
static unsigned char *load(const char *path,size_t *n){
 FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
 long l=ftell(f);
 if(l<0||l>32*1024*1024L||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 unsigned char *d=(unsigned char*)malloc((size_t)l+1u);
 if(!d){fclose(f);return NULL;}
 size_t read=fread(d,1,(size_t)l,f);
 fclose(f);
 if(read!=(size_t)l){free(d);return NULL;}
 *n=(size_t)l;return d;
}
int main(int argc,char **argv){
 cnk_brew_mod_info mod={0};
 cnk_brew_mif_info mif={0};
 unsigned char raw[0x80]={0};
 assert(cnk_brew_mod_inspect(NULL,0,&mod)<0);
 assert(cnk_brew_mod_inspect(raw,sizeof(raw),NULL)<0);
 /* Real Crash raw pattern: first ARM B jumps from 0 to 0x14.
  * Zeebx starts at 0 so the CPU executes this branch itself. */
 wr32(raw,0xea000003u);
 assert(!cnk_brew_mod_inspect(raw,sizeof(raw),&mod));
 assert(!mod.has_brew_header && mod.has_initial_branch);
 assert(mod.entry_offset==0u && mod.first_branch_target==0x14u);
 wr32(raw,0xea00000eu);
 memcpy(raw+8,"BREW",4);
 wr32(raw+12,1);wr32(raw+16,0x40u);
 wr32(raw+20,0x101u);
 assert(!cnk_brew_mod_inspect(raw,sizeof(raw),&mod));
 assert(mod.has_brew_header && mod.entry_offset==0x40u);
 assert(mod.code_offset==0x40u && mod.brew_version==1u);
 assert(mod.header_words[0]==0x101u);
 /* Both tests must fail atomically and never leak a half-parsed header. */
 unsigned char bad[0x80]={0};
 memcpy(bad,raw,sizeof(raw));
 wr32(bad+16,0x44); /* offset is aligned, but entry < code offset */
 assert(cnk_brew_mod_inspect(bad,sizeof(bad),&mod)<0);
 assert(!mod.image_bytes && !mod.has_brew_header);
 wr32(bad+16,0x40);
 assert(cnk_brew_mod_inspect(bad,0x1f,&mod)<0);
 assert(!mod.image_bytes);
 memset(bad,0,sizeof(bad));
 wr16(bad,0x11);
 wr32(bad+0x10,0x20);wr32(bad+0x14,2);
 wr32(bad+0x20,0x40);wr32(bad+0x24,0x54);
 wr32(bad+0x28,0x5c);
 /* Applet record: 20-byte clsid at 0x40, zeros at 0x44/0x4c. */
 wr32(bad+0x40,0x01081984u);
 /* 8-byte class dependency at 0x54 */
 wr32(bad+0x54,0x01020203u);
 assert(!cnk_brew_mif_inspect(bad,sizeof(bad),&mif));
 assert(mif.sections==2u && mif.main_applet==0x01081984u);
 assert(mif.applet_count==1u&&mif.class_count==1u);
 assert(mif.classes[0]==0x01020203u);
 wr32(bad+0x4c,0x10u);
 assert(!cnk_brew_mif_inspect(bad,sizeof(bad),&mif));
 assert(!mif.applet_count && mif.class_count==1u);
 wr32(bad+0x4c,0u);
 wr32(bad+0x24,0x22); /* descending nonmonotonic section table */
 assert(cnk_brew_mif_inspect(bad,sizeof(bad),&mif)<0);
 assert(!mif.sections);
 wr32(bad+0x24,0x54);
 wr32(bad+0x14,UINT32_MAX);
 assert(cnk_brew_mif_inspect(bad,sizeof(bad),&mif)<0);
 if(argc>=3){
  size_t n=0;
  unsigned char *d=load(argv[1],&n);
  assert(d && !cnk_brew_mod_inspect(d,n,&mod));
  /* These facts were observed in the user's unmodified cnk2.mod;
   * no binary data or test fixture is uploaded to GitHub. */
  assert(n==649452u && !mod.has_brew_header);
  assert(mod.has_initial_branch && mod.first_branch_target==0x14u);
  free(d);
  d=load(argv[2],&n);
  assert(d && !cnk_brew_mif_inspect(d,n,&mif));
  assert(mif.sections==10u && mif.applet_count>=1u);
  assert(mif.main_applet==0x01081984u);
  free(d);
  puts("PASS: source CNK mod and MIF identities cross-checked");
 }
 puts("PASS: Zeebx-adapted BREW .mod/.mif metadata, bounded failures");
 return 0;
}
