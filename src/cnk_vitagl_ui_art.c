#include "cnk_vitagl_ui_art.h"
#include <vitaGL.h>
#include <png.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void cnk_vitagl_ui_art_free(cnk_vitagl_ui_art *art){
 if(!art)return;
 for(unsigned i=0;i<CNK_SOURCE_UI_COUNT;i++){
  if(art->handles[i]){
   GLuint handle=(GLuint)art->handles[i];
   glDeleteTextures(1,&handle);
  }
 }
 memset(art,0,sizeof(*art));
}
int cnk_vitagl_ui_art_load(cnk_vitagl_ui_art *out,const fufs_file *vfs){
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!vfs)return -1;
 for(unsigned i=0;i<CNK_SOURCE_UI_COUNT;i++){
  unsigned char *source=NULL,*rgba=NULL;
  size_t source_len=0,removed=0;
  png_image image={0};
  const cnk_source_ui_spec *spec=cnk_source_ui_original(i);
  GLuint handle=0;
  /* The PNG source index, ID, dimensions and CRC MUST all match the
   * original version before anything is uploaded to the GPU. */
  if(cnk_source_ui_extract(vfs,i,&source,&source_len))goto fail;
  image.version=PNG_IMAGE_VERSION;
  if(!png_image_begin_read_from_memory(&image,source,source_len))
   goto fail_image;
  if(image.width!=spec->width||image.height!=spec->height||
     image.width>4096u||image.height>4096u)goto fail_image;
  image.format=PNG_FORMAT_RGBA;
  size_t bytes=(size_t)PNG_IMAGE_SIZE(image);
  if(!bytes||bytes>4u*1024u*1024u)goto fail_image;
  rgba=(unsigned char*)malloc(bytes);
  if(!rgba)goto fail_image;
  if(!png_image_finish_read(&image,NULL,rgba,0,NULL))goto fail_image;
  if(cnk_source_ui_key_magenta(rgba,(size_t)image.width*image.height,
                               bytes,&removed)||!removed)goto fail_image;
  png_image_free(&image);
  free(source);source=NULL;
  glGenTextures(1,&handle);
  if(!handle)goto fail_image;
  glBindTexture(GL_TEXTURE_2D,handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,(GLsizei)spec->width,
               (GLsizei)spec->height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
  free(rgba);rgba=NULL;
  if(glGetError()!=GL_NO_ERROR)goto fail_image;
  out->handles[i]=handle;
  glBindTexture(GL_TEXTURE_2D,0);
  continue;
fail_image:
  png_image_free(&image);
  free(source);free(rgba);
  if(handle)glDeleteTextures(1,&handle);
fail:
  cnk_vitagl_ui_art_free(out);
  return -2;
 }
 return 0;
}
int cnk_vitagl_ui_art_draw(const cnk_vitagl_ui_art *art,unsigned kind,
                           float x,float y,float w,float h){
 if(!art||kind>=CNK_SOURCE_UI_COUNT||!art->handles[kind]||
    !(w>0.f&&h>0.f))return -1;
 const float pos[12]={
  x,y, x+w,y, x+w,y+h,
  x,y, x+w,y+h, x,y+h
 };
 const float uv[12]={
  0.f,0.f,1.f,0.f,1.f,1.f,
  0.f,0.f,1.f,1.f,0.f,1.f
 };
 glEnable(GL_BLEND);
 glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 glEnable(GL_TEXTURE_2D);
 glBindTexture(GL_TEXTURE_2D,(GLuint)art->handles[kind]);
 glColor4f(1.f,1.f,1.f,1.f);
 glEnableClientState(GL_VERTEX_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 glVertexPointer(2,GL_FLOAT,0,pos);
 glTexCoordPointer(2,GL_FLOAT,0,uv);
 glDrawArrays(GL_TRIANGLES,0,6);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 glDisableClientState(GL_VERTEX_ARRAY);
 glBindTexture(GL_TEXTURE_2D,0);
 glDisable(GL_TEXTURE_2D);
 glDisable(GL_BLEND);
 return 0;
}
