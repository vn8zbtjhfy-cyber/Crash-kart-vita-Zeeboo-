#include "cnk_pfnt.h"
#include <limits.h>
#include <string.h>

#define CNK_PFNT_FIXED_BYTES 18u
#define CNK_PFNT_MAX_GLYPHS 2048u
static uint16_t rd16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd24(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16);}

int cnk_pfnt_open(const uint8_t *data,size_t size,cnk_pfnt_view *out){
  uint16_t first,last,count,prev=0;
  size_t metrics_start,metrics_end;
  uint32_t running=0;
  uint16_t i;
  if(!out)return -1;
  memset(out,0,sizeof(*out));
  if(!data||size<CNK_PFNT_FIXED_BYTES||memcmp(data,"PFNT",4))return -2;
  /* Byte 4 is the source's 12-byte header marker; the undecoded bytes 6-11
   * are intentionally not interpreted as flags, color or font roles. */
  if(rd16(data+4)!=12u||!data[9]||data[9]>128u)return -3;
  first=rd16(data+12);last=rd16(data+14);count=rd16(data+16);
  if(!count||count>CNK_PFNT_MAX_GLYPHS||last<first ||
     (uint32_t)last-(uint32_t)first+1u<count)return -4;
  metrics_start=CNK_PFNT_FIXED_BYTES+(size_t)count*2u;
  metrics_end=metrics_start+(size_t)count*4u;
  /* An index alone is not an entire PFNT. Require additional encoded content. */
  if(size<metrics_end+16u)return -6;
  for(i=0;i<count;i++){
    uint16_t cp=rd16(data+CNK_PFNT_FIXED_BYTES+(size_t)i*2u);
    const uint8_t *m=data+metrics_start+(size_t)i*4u;
    uint32_t x=rd24(m);uint32_t width=m[3];
    if(cp<first||cp>last||(i&&cp<=prev))return -7;
    if(!width||x!=running||running>UINT32_MAX-width)return -8;
    running+=width;prev=cp;
  }
  if(rd16(data+CNK_PFNT_FIXED_BYTES)!=first||prev!=last)return -9;
  out->bytes=data;out->length=size;out->codepoint_first=first;
  out->codepoint_last=last;out->glyph_count=count;
  out->line_height_px=data[9];out->total_advance_px=running;
  out->trailing_payload_offset=metrics_end;
  return 0;
}
int cnk_pfnt_glyph_for(const cnk_pfnt_view *font,uint32_t codepoint,cnk_pfnt_glyph *out){
  size_t lo=0,hi,mid,metrics;
  if(!font||!font->bytes||!out)return -1;
  if(codepoint<font->codepoint_first||codepoint>font->codepoint_last)return 1;
  hi=font->glyph_count;metrics=CNK_PFNT_FIXED_BYTES+(size_t)font->glyph_count*2u;
  while(lo<hi){
    uint16_t cp;
    mid=lo+(hi-lo)/2;
    cp=rd16(font->bytes+CNK_PFNT_FIXED_BYTES+mid*2u);
    if(codepoint>cp)lo=mid+1;
    else hi=mid;
  }
  if(lo>=font->glyph_count || rd16(font->bytes+CNK_PFNT_FIXED_BYTES+lo*2u)!=codepoint)return 1;
  const uint8_t *m=font->bytes+metrics+lo*4u;
  out->codepoint=(uint16_t)codepoint;out->atlas_x_px=rd24(m);out->advance_px=m[3];
  return 0;
}
static int read_utf8(const uint8_t *p,size_t n,size_t *used,uint32_t *cp){
  uint32_t c,min_cp;size_t j,needed;
  if(!n)return -1;
  if(p[0]<0x80u){*used=1;*cp=p[0];return 0;}
  if(p[0]>=0xc2u&&p[0]<=0xdfu){needed=2;min_cp=0x80u;c=p[0]&0x1fu;}
  else if(p[0]>=0xe0u&&p[0]<=0xefu){needed=3;min_cp=0x800u;c=p[0]&0x0fu;}
  else if(p[0]>=0xf0u&&p[0]<=0xf4u){needed=4;min_cp=0x10000u;c=p[0]&0x07u;}
  else return -1;
  if(n<needed)return -1;
  for(j=1;j<needed;j++){
    if((p[j]&0xc0u)!=0x80u)return -1;
    c=(c<<6)|(p[j]&0x3fu);
  }
  if(c<min_cp||c>0x10ffffu||(c>=0xd800u&&c<=0xdfffu))return -1;
  *used=needed;*cp=c;return 0;
}
int cnk_pfnt_measure_utf8(const cnk_pfnt_view *font,const char *text,size_t text_bytes,uint32_t *width_px){
  size_t pos=0;uint32_t total=0;
  if(!font||!font->bytes||!text||!width_px)return -1;
  /* Leave output untouched on failure. */
  while(pos<text_bytes){
    uint32_t cp;size_t used;cnk_pfnt_glyph glyph;
    int status=read_utf8((const uint8_t*)text+pos,text_bytes-pos,&used,&cp);
    if(status)return -4;
    status=cnk_pfnt_glyph_for(font,cp,&glyph);
    if(status)return status==1?-3:-2;
    if(total>UINT32_MAX-glyph.advance_px)return -5;
    total+=glyph.advance_px;pos+=used;
  }
  *width_px=total;return 0;
}

int cnk_pfnt_encoded_atlas_open(const cnk_pfnt_view *font,
                                cnk_pfnt_encoded_atlas *out){
  size_t count, offset;
  const uint8_t *p;
  if(!out)return -1;
  memset(out,0,sizeof(*out));
  if(!font||!font->bytes || !font->line_height_px ||
     !font->total_advance_px || font->total_advance_px>UINT16_MAX ||
     font->trailing_payload_offset>font->length)return -2;
  /* All five source PFNTs have a tagged bitmap record at the documented
   * trailing offset. Its 16-bit dimensions match actual PFNT metrics.
   * The byte stream itself is not assumed to be a usable grayscale atlas. */
  p=font->bytes+font->trailing_payload_offset;
  if(font->length-font->trailing_payload_offset<12u || p[0]!=3u ||
     rd16(p+4)!=(uint16_t)font->total_advance_px ||
     rd16(p+6)!=(uint16_t)font->line_height_px)return -3;
  count=(size_t)font->total_advance_px*(size_t)font->line_height_px;
  if(count>font->length-font->trailing_payload_offset)return -4;
  offset=font->length-count;
  if(offset<font->trailing_payload_offset+12u)return -5;
  out->stored_width_px=(uint16_t)font->total_advance_px;
  out->stored_height_px=(uint16_t)font->line_height_px;
  out->image_preamble=p;
  out->preamble_bytes=offset-font->trailing_payload_offset;
  out->encoded_pixels=font->bytes+offset;
  out->encoded_bytes=count;
  return 0;
}

/* UTF-8 decoder above rejects invalid/overlong codepoints. The first pass
 * validates and measures everything, preventing partially emitted glyphs. */
int cnk_pfnt_layout_utf8(const cnk_pfnt_view *font,const char *text,
                          size_t text_bytes,cnk_pfnt_positioned_glyph *glyphs,
                          size_t capacity,size_t *glyph_count,
                          uint32_t *text_width_px,uint32_t *text_height_px){
  size_t i,pos=0,n=0;
  uint32_t width=0,maximum=0,lines=1,height;
  if(!font||!font->bytes||!font->line_height_px||!text||
     !glyph_count||!text_width_px||!text_height_px||
     (capacity&&!glyphs))return -1;
  for(i=0;i<2;i++){
    pos=0;width=0;maximum=0;lines=1;n=0;
    while(pos<text_bytes){
      uint32_t cp;size_t used;cnk_pfnt_glyph g;
      int r=read_utf8((const uint8_t *)text+pos,text_bytes-pos,&used,&cp);
      if(r)return -4;
      if(cp==10u){
        if(width>maximum)maximum=width;
        if(lines>UINT32_MAX/font->line_height_px-1u)return -5;
        lines++;width=0;
      } else {
        r=cnk_pfnt_glyph_for(font,cp,&g);
        if(r)return r==1?-3:-2;
        if(width>UINT32_MAX-g.advance_px)return -5;
        if(n==SIZE_MAX)return -5;
        if(i==1){
          glyphs[n].codepoint=cp;
          glyphs[n].source_x_px=g.atlas_x_px;
          glyphs[n].dest_x_px=width;
          glyphs[n].dest_y_px=(lines-1u)*font->line_height_px;
          glyphs[n].advance_px=g.advance_px;
        }
        n++;width+=g.advance_px;
      }
      pos+=used;
    }
    if(width>maximum)maximum=width;
    if(i==0 && n>capacity)return -6;
  }
  height=lines*font->line_height_px;
  *glyph_count=n;
  *text_width_px=maximum;
  *text_height_px=height;
  return 0;
}
