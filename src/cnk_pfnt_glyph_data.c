#include "cnk_pfnt_glyph_data.h"
#include <string.h>

int cnk_pfnt_encoded_glyph_get(const cnk_pfnt_view *font,
                               uint32_t codepoint,
                               cnk_pfnt_encoded_glyph *out){
    cnk_pfnt_encoded_atlas atlas;
    cnk_pfnt_glyph g;
    cnk_pfnt_encoded_glyph result;
    int status;
    if(!font||!out)return -1;
    /* Re-open bitmap bounds instead of trusting an external pointer. */
    status=cnk_pfnt_encoded_atlas_open(font,&atlas);
    if(status)return -2;
    status=cnk_pfnt_glyph_for(font,codepoint,&g);
    if(status)return status==1?1:-3;
    if((uint32_t)g.advance_px > atlas.stored_width_px ||
       g.atlas_x_px > (uint32_t)atlas.stored_width_px-g.advance_px ||
       atlas.encoded_bytes!=(size_t)atlas.stored_width_px*atlas.stored_height_px)return -4;
    result.codepoint=codepoint;
    result.atlas_x_bytes=g.atlas_x_px;
    result.width_bytes=g.advance_px;
    result.rows=atlas.stored_height_px;
    result.atlas_row_stride_bytes=atlas.stored_width_px;
    result.atlas_first_byte=atlas.encoded_pixels;
    *out=result;
    return 0;
}

size_t cnk_pfnt_encoded_glyph_bytes(const cnk_pfnt_encoded_glyph *glyph){
    if(!glyph||!glyph->width_bytes||!glyph->rows||
       glyph->width_bytes>glyph->atlas_row_stride_bytes||
       glyph->atlas_x_bytes>(uint32_t)glyph->atlas_row_stride_bytes-glyph->width_bytes||
       !glyph->atlas_first_byte)return 0;
    return (size_t)glyph->width_bytes*glyph->rows;
}

int cnk_pfnt_encoded_glyph_copy(const cnk_pfnt_encoded_glyph *glyph,
                                uint8_t *dst,size_t dst_capacity){
    size_t needed=cnk_pfnt_encoded_glyph_bytes(glyph);
    if(!needed||!dst)return -1;
    if(dst_capacity<needed)return -2;
    for(size_t row=0;row<glyph->rows;row++){
        memcpy(dst+(size_t)row*glyph->width_bytes,
               glyph->atlas_first_byte+(size_t)row*glyph->atlas_row_stride_bytes+glyph->atlas_x_bytes,
               glyph->width_bytes);
    }
    return 0;
}
