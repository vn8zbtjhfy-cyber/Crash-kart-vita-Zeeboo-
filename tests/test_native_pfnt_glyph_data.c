#include "fufs.h"
#include "cnk_pfnt.h"
#include "cnk_pfnt_glyph_data.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv){
    fufs_file vfs={0};size_t fonts=0,glyphs=0;
    if(argc!=2)return 2;
    assert(fufs_open(argv[1],&vfs)==0);
    for(uint32_t i=0;i<vfs.count;i++){
        unsigned char *data=NULL;
        size_t size=0;
        cnk_pfnt_view f={0};cnk_pfnt_encoded_atlas atlas={0};
        assert(fufs_extract(&vfs,i,&data,&size)==0);
        if(strcmp(fufs_type(data,size),"pfnt")){free(data);continue;}
        fonts++;
        assert(cnk_pfnt_open(data,size,&f)==0);
        assert(cnk_pfnt_encoded_atlas_open(&f,&atlas)==0);
        uint8_t *rebuilt=(uint8_t*)calloc(atlas.encoded_bytes,1);
        uint8_t *scratch=(uint8_t*)malloc(atlas.encoded_bytes+1u);
        assert(rebuilt&&scratch);
        memset(scratch,0x5a,atlas.encoded_bytes+1u);
        for(uint32_t j=0;j<f.glyph_count;j++){
            cnk_pfnt_encoded_glyph g={0};
            uint32_t cp=data[18u+j*2u] | ((uint32_t)data[19u+j*2u]<<8);
            size_t n;
            assert(cnk_pfnt_encoded_glyph_get(&f,cp,&g)==0);
            assert(g.codepoint==cp);
            assert(g.atlas_row_stride_bytes==atlas.stored_width_px);
            assert(g.rows==atlas.stored_height_px);
            n=cnk_pfnt_encoded_glyph_bytes(&g);
            assert(n==(size_t)g.width_bytes*g.rows);
            assert(cnk_pfnt_encoded_glyph_copy(&g,scratch,n)==0);
            for(size_t y=0;y<g.rows;y++){
                const uint8_t *expected=atlas.encoded_pixels+y*atlas.stored_width_px+g.atlas_x_bytes;
                assert(memcmp(scratch+y*g.width_bytes,expected,g.width_bytes)==0);
                memcpy(rebuilt+y*atlas.stored_width_px+g.atlas_x_bytes,scratch+y*g.width_bytes,g.width_bytes);
            }
            /* Errors are atomic: caller's bytes, including guard, stay intact. */
            memset(scratch,0x5a,n+1u);
            assert(cnk_pfnt_encoded_glyph_copy(&g,scratch,n-1u)<0);
            for(size_t k=0;k<=n;k++)assert(scratch[k]==0x5a);
            assert(cnk_pfnt_encoded_glyph_copy(&g,NULL,n)<0);
            glyphs++;
        }
        assert(memcmp(rebuilt,atlas.encoded_pixels,atlas.encoded_bytes)==0);
        cnk_pfnt_encoded_glyph missing={0};
        memset(&missing,0x42,sizeof(missing));
        cnk_pfnt_encoded_glyph original=missing;
        assert(cnk_pfnt_encoded_glyph_get(&f,0x110000u,&missing)!=0);
        assert(memcmp(&missing,&original,sizeof(missing))==0);
        cnk_pfnt_view wrong=f;wrong.total_advance_px=70000;
        assert(cnk_pfnt_encoded_glyph_get(&wrong,f.codepoint_first,&missing)<0);
        assert(memcmp(&missing,&original,sizeof(missing))==0);
        cnk_pfnt_encoded_glyph bad={0};
        assert(cnk_pfnt_encoded_glyph_bytes(&bad)==0);
        assert(cnk_pfnt_encoded_glyph_copy(&bad,scratch,atlas.encoded_bytes)<0);
        free(scratch);free(rebuilt);free(data);
    }
    fufs_close(&vfs);
    assert(fonts==5&&glyphs==348);
    puts("All 348 original encoded PFNT glyphs reconstruct five backing spans byte-for-byte");
    return 0;
}
