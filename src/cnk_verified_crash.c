#include "cnk_verified_crash.h"
#include <string.h>
/* Source-backed unique Crash CHARACTER, not a kart: exact model and PNG ID. */
void cnk_verified_crash_free(cnk_verified_crash *c){
 if(!c)return;
 cnk_object_skin_free(&c->original);
 memset(c,0,sizeof(*c));
}
unsigned int cnk_verified_crash_resolve(void *ctx,uint32_t selector){
 const cnk_verified_crash *c=(const cnk_verified_crash*)ctx;
 return cnk_object_skin_resolve(c?(void*)&c->original:NULL,selector);
}
int cnk_verified_crash_load(const fufs_file *vfs,const cnk_png_catalog *cat,cnk_verified_crash *out){
 if(!out||!vfs||!cat)return -1;
 memset(out,0,sizeof(*out));
 const cnk_png_resource *png=cnk_png_find_index(cat,365);
 if(!png||png->vfs_id!=0xee898141u||png->image.width!=256||png->image.height!=256)return -2;
 cnk_object_skin_reference mapping={"Crash.png",365u,0xee898141u};
 return cnk_object_skin_load(vfs,cat,19u,0x0845c03fu,&mapping,1,&out->original);
}
