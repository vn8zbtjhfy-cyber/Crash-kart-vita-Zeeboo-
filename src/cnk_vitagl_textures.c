#include "cnk_vitagl_textures.h"
#include <vitaGL.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
int cnk_vitagl_textures_init(cnk_vitagl_textures *set,const fufs_file *vfs,
                             const cnk_png_catalog *cat,uint32_t count){
 if(!set)return -1;
 memset(set,0,sizeof(*set));
 if(!vfs||!cat||!count||count>1024)return -2;
 set->handles=(unsigned int*)calloc(count,sizeof(*set->handles));
 if(!set->handles)return -3;
 set->vfs=vfs;set->catalog=cat;set->count=count;
 return 0;
}
unsigned int cnk_vitagl_textures_resolve(void *ptr,uint16_t index){
 const cnk_vitagl_textures *s=(const cnk_vitagl_textures*)ptr;
 if(!s||index>=s->count||!s->handles)return 0;
 return s->handles[index];
}
int cnk_vitagl_textures_upload(cnk_vitagl_textures *s,cnk_texture_binding bind){
 unsigned char *compressed=NULL,*rgba=NULL;size_t size=0;
 png_image image;GLuint handle=0;
 const cnk_png_resource *orig;
 if(!s||!s->handles||!s->catalog||!s->vfs||!s->vfs->entries||
    bind.texture_index>=s->count||bind.vfs_index>=s->vfs->count)return -1;
 orig=cnk_png_find_index(s->catalog,bind.vfs_index);
 if(!orig||orig->vfs_id!=s->vfs->entries[bind.vfs_index].id)return -2;
 /* Refuse to silently replace a texture bound to a different file. */
 if(s->handles[bind.texture_index])return -3;
 if(fufs_extract(s->vfs,bind.vfs_index,&compressed,&size))return -4;
 memset(&image,0,sizeof(image));image.version=PNG_IMAGE_VERSION;
 if(!png_image_begin_read_from_memory(&image,compressed,size)){
  free(compressed);return -5;
 }
 if(image.width!=orig->image.width||image.height!=orig->image.height ||
    image.width>4096||image.height>4096){
  png_image_free(&image);free(compressed);return -6;
 }
 image.format=PNG_FORMAT_RGBA;
 if(PNG_IMAGE_SIZE(image)>32u*1024u*1024u){
  png_image_free(&image);free(compressed);return -7;
 }
 rgba=(unsigned char*)malloc(PNG_IMAGE_SIZE(image));
 if(!rgba){png_image_free(&image);free(compressed);return -8;}
 if(!png_image_finish_read(&image,NULL,rgba,0,NULL)){
  png_image_free(&image);free(rgba);free(compressed);return -9;
 }
 png_image_free(&image);free(compressed);
 glGenTextures(1,&handle);
 if(!handle){free(rgba);return -10;}
 glBindTexture(GL_TEXTURE_2D,handle);
 glPixelStorei(GL_UNPACK_ALIGNMENT,1);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 /* UV wrap/mirroring and alpha blending are not yet validated against BREW. */
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,(GLsizei)orig->image.width,
              (GLsizei)orig->image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
 GLenum glstatus=glGetError();
 free(rgba);
 if(glstatus!=GL_NO_ERROR){glDeleteTextures(1,&handle);return -11;}
 s->handles[bind.texture_index]=handle;
 return 0;
}
void cnk_vitagl_textures_free(cnk_vitagl_textures *s){
 if(!s)return;
 if(s->handles){
  for(uint32_t i=0;i<s->count;i++)if(s->handles[i]){
   GLuint h=(GLuint)s->handles[i];glDeleteTextures(1,&h);
  }
 }
 free(s->handles);memset(s,0,sizeof(*s));
}
