/* Zeebx IDisplay -> IBitmap -> IDIB ABI exercised by guest ARM
 * instructions, using only a synthetic memory image (NO ROM). */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void wr(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(i*8u));
}
int main(void){
 uint8_t image[160]={0};
 wr(image+0,0xe1a0400eu);  /* MOV r4,lr */
 wr(image+4,0xe1a05001u);  /* MOV r5,r1: GetDeviceBitmap result */
 wr(image+8,0xe1a06002u);  /* MOV r6,r2: QueryInterface result */
 wr(image+12,0xe5903000u); /* LDR r3,[r0] (IDisplay vtable) */
 wr(image+16,0xe5933040u); /* LDR r3,[r3,#64] (Display slot 16) */
 wr(image+20,0xe12fff33u); /* BLX r3 */
 wr(image+24,0xe5950000u); /* LDR r0,[r5] (original Bitmap*) */
 wr(image+28,0xe5903000u); /* LDR r3,[r0] (IBitmap vtable) */
 wr(image+32,0xe5933008u); /* LDR r3,[r3,#8] (QueryInterface) */
 wr(image+36,0xe59f1014u); /* LDR r1,[pc,#20] -> IDIB ClassID */
 wr(image+40,0xe1a02006u); /* MOV r2,r6 */
 wr(image+44,0xe12fff33u); /* BLX r3 */
 wr(image+48,0xe1a0e004u); /* MOV lr,r4 */
 wr(image+52,0xe12fff1eu); /* BX lr */
 wr(image+64,0x01001045u); /* AEECLSID_DIB */
 cnk_zeebx_arm c={0};
 cnk_arm_outcome out={0};
 uint32_t handled=0,display=0,bitmap=0,dib=0,value=0,original_size=0;
 assert(!cnk_zeebx_arm_init(&c,image,sizeof(image)));
 /* Create IDISPLAY through the actual Zeebx BREW factory. */
 c.regs[0]=CNK_ARM_OBJECT_BASE;
 c.regs[1]=0x01001001u;
 c.regs[2]=CNK_ARM_OBJECT_BASE+0x100u;
 c.regs[14]=CNK_ARM_RETURN_MAGIC;
 c.regs[15]=CNK_ARM_API_BASE+8u;
 assert(!cnk_zeebx_arm_bootstrap(&c,20u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(!cnk_zeebx_arm_read_u32(&c,CNK_ARM_OBJECT_BASE+0x100u,&display));
 assert(display==CNK_ARM_OBJECT_BASE+16u);
 /* Guest A32 performs two real virtual function calls, not C callbacks. */
 c.regs[0]=display;
 c.regs[1]=CNK_ARM_OBJECT_BASE+0x104u;
 c.regs[2]=CNK_ARM_OBJECT_BASE+0x108u;
 c.regs[14]=CNK_ARM_RETURN_MAGIC;
 c.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&c,150u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED&&handled==2u);
 assert(!cnk_zeebx_arm_read_u32(&c,CNK_ARM_OBJECT_BASE+0x104u,&bitmap));
 assert(!cnk_zeebx_arm_read_u32(&c,CNK_ARM_OBJECT_BASE+0x108u,&dib));
 assert(bitmap==dib && bitmap==c.screen_bitmap_addr && bitmap!=0);
 assert(c.screen_bitmap_references==3u);
 assert(!cnk_zeebx_arm_read_u32(&c,bitmap,&value));
 assert(value==CNK_ARM_VTABLE_BASE+7u*CNK_ARM_VTABLE_STRIDE);
 assert(!cnk_zeebx_arm_read_u32(&c,bitmap+8u,&value));
 assert(value==c.screen_bitmap_pixels && value==CNK_ARM_HEAP_BASE);
 assert(!cnk_zeebx_arm_read_u32(&c,bitmap+16u,&value));
 assert(value==0xf81fu);
 assert(!cnk_zeebx_arm_read_u32(&c,bitmap+20u,&value));
 assert(value==(640u|(480u<<16)));
 assert(!cnk_zeebx_arm_read_u32(&c,bitmap+24u,&value));
 assert(value==1280u);
 assert(c.objects[bitmap-CNK_ARM_OBJECT_BASE+28u]==16u);
 assert(c.objects[bitmap-CNK_ARM_OBJECT_BASE+29u]==16u);
 assert(c.heap_cursor==640u*480u*2u);
 assert(c.heap[0]==0u&&c.heap[c.heap_cursor-1u]==0u);
 original_size=c.heap_cursor;
 /* Same bitmap object and pixels are returned again; no heap leak. */
 c.regs[0]=display;c.regs[1]=CNK_ARM_OBJECT_BASE+0x10cu;
 c.regs[14]=CNK_ARM_RETURN_MAGIC;
 c.regs[15]=CNK_ARM_API_BASE+5u*4096u+16u*4u;
 assert(!cnk_zeebx_arm_bootstrap(&c,10u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(c.screen_bitmap_addr==bitmap&&c.heap_cursor==original_size);
 /* Reject unknown bitmap interface instead of fabricating an IDIB. */
 c.regs[0]=bitmap;c.regs[1]=0x0100beefu;c.regs[2]=CNK_ARM_OBJECT_BASE+0x110u;
 c.regs[14]=CNK_ARM_RETURN_MAGIC;c.regs[15]=CNK_ARM_API_BASE+7u*4096u+8u;
 assert(!cnk_zeebx_arm_bootstrap(&c,20u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_interface==7u&&out.api_slot==2u);
 /* Persistent device surface refuses to release its owner's last ref. */
 for(unsigned i=0;i<5u;i++){
  c.regs[0]=bitmap;c.regs[14]=CNK_ARM_RETURN_MAGIC;
  c.regs[15]=CNK_ARM_API_BASE+7u*4096u+4u;
  assert(!cnk_zeebx_arm_bootstrap(&c,10u,&out,&handled));
  assert(out.reason==CNK_ARM_RETURNED);
 }
 assert(c.screen_bitmap_references==1u && c.heap_cursor==original_size);
 cnk_zeebx_arm_destroy(&c);
 puts("PASS: Zeebx ARM GetDeviceBitmap/IDIB QueryInterface real guest framebuffer, RGB565, persistent refs and unknown-IID stops");
 return 0;
}
