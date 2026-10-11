/* Source-assisted identification of Crash's CHARACTER texture, not a kart.
 * ResHax 2024 community report names ee898141.PLZP as "Crash texture".
 * This test ensures it is present in THIS user-provided Zeebo ROM, is a
 * valid PNG, and that model #19 actually requests a texture named Crash.png.
 * It does NOT assert that a general filename->FUFS hash is known.
 */
#include "cnk_model_material.h"
#include "cnk_png.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int argc,char **argv){
    assert(argc>=2);
    fufs_file vfs={0};
    assert(fufs_open(argv[1],&vfs)==0);
    assert(vfs.count==412);
    assert(vfs.entries[365].id==0xee898141u);
    const unsigned char *packed=vfs.data+vfs.entries[365].offset;
    assert(vfs.entries[365].size>=12);
    assert(memcmp(packed,"PLZP",4)==0);
    unsigned char *png_data=NULL;size_t png_size=0;
    assert(fufs_extract(&vfs,365,&png_data,&png_size)==0);
    cnk_png_info info={0};
    assert(cnk_png_inspect(png_data,png_size,&info)==0);
    assert(info.width==256 && info.height==256);
    free(png_data);
    cnk_material_reference mat[512]={{0}};
    unsigned groups=0;
    assert(cnk_model_material_from_vfs(&vfs,19,mat,512,&groups)==0);
    assert(groups==3);
    int found=0;
    for(unsigned i=0;i<groups;i++){
        if(!strcmp(mat[i].source_png,"Crash.png"))found++;
    }
    assert(found==1);
    printf("Crash character texture evidence: model #19 says Crash.png; "
           "external ee898141.PLZP = VFS #365; PNG 256x256; "
           "pair remains source-assisted, not generic name-hash proof.\n");
    fufs_close(&vfs);
    return 0;
}
