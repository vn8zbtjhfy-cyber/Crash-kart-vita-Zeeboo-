#include "cnk_texture_manifest.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

void cnk_texture_manifest_free(cnk_texture_manifest *m){
 if(!m)return;
 free(m->rows);memset(m,0,sizeof(*m));
}
static int dec32(const char *s,uint32_t *out){
 uint32_t n=0;
 if(!s||!*s)return -1;
 for(;*s;s++){
  if(*s<'0'||*s>'9')return -1;
  uint32_t digit=(uint32_t)(*s-'0');
  if(n>(UINT32_MAX-digit)/10u)return -1;
  n=n*10u+digit;
 }
 *out=(uint32_t)n;return 0;
}
static int hex32(const char *s,uint32_t *out){
 uint32_t n=0;
 if(!s||strlen(s)!=8)return -1;
 for(int i=0;i<8;i++){
  unsigned c=(unsigned char)s[i];
  if(c>='0'&&c<='9')c-='0';
  else if(c>='A'&&c<='F')c=c-'A'+10;
  else if(c>='a'&&c<='f')c=c-'a'+10;
  else return -1;
  n=(n<<4)|c;
 }
 *out=n;return 0;
}
static int parse_line(char *line,cnk_manifest_row *r){
 char *fields[6];int n=0;char *p=line;
 /* There is deliberately no CSV quoting or automatic trimming: names
    must match the original scene byte for byte. */
 fields[n++]=p;
 while(*p){
  if(*p=='\t'){
   if(n==6)return -1;
   *p='\0';fields[n++]=p+1;
  }
  p++;
 }
 if(n!=6)return -1;
 uint32_t index=0,slot=0,pngidx=0;
 if(dec32(fields[0],&index)||hex32(fields[1],&r->scene_resource_id)||
    dec32(fields[2],&slot)||slot>UINT16_MAX||
    !fields[3][0]||strlen(fields[3])>=CNK_MANIFEST_NAME_CAP||
    dec32(fields[4],&pngidx)||hex32(fields[5],&r->png_resource_id))return -1;
 for(const unsigned char *c=(const unsigned char*)fields[3];*c;c++){
  if(*c<' '||*c>126||*c=='\\')return -1;
 }
 r->scene_vfs_index=index;r->texture_slot=(uint16_t)slot;
 r->png_vfs_index=pngidx;
 strcpy(r->original_texture_name,fields[3]);
 return 0;
}
int cnk_texture_manifest_read(const char *path,cnk_texture_manifest *out){
 FILE *f; cnk_texture_manifest m={0};char line[512];int error=0;
 if(!out||!path||!*path)return -1;
 memset(out,0,sizeof(*out));
 f=fopen(path,"rb");
 if(!f)return errno==ENOENT?-2:-3;
 while(fgets(line,sizeof(line),f)){
  size_t len=strlen(line);
  if(len==sizeof(line)-1&&line[len-1]!='\n'){error=-4;break;}
  if(len&&line[len-1]=='\n')line[--len]=0;
  if(len&&line[len-1]=='\r')line[--len]=0;
  if(!len||line[0]=='#')continue;
  if(m.count==CNK_MANIFEST_MAX_REFS){error=-5;break;}
  cnk_manifest_row row={0};
  if(parse_line(line,&row)){error=-6;break;}
  for(size_t i=0;i<m.count;i++){
   cnk_manifest_row *a=&m.rows[i];
   if((a->scene_vfs_index==row.scene_vfs_index && a->texture_slot==row.texture_slot) ||
      (!strcmp(a->original_texture_name,row.original_texture_name) &&
       (a->png_vfs_index!=row.png_vfs_index||a->png_resource_id!=row.png_resource_id)) ||
      (a->png_resource_id==row.png_resource_id&&a->png_vfs_index!=row.png_vfs_index)){
    error=-7;break;
   }
  }
  if(error)break;
  cnk_manifest_row *newrows=(cnk_manifest_row*)realloc(m.rows,(m.count+1)*sizeof(*newrows));
  if(!newrows){error=-8;break;}
  m.rows=newrows;m.rows[m.count++]=row;
 }
 if(ferror(f))error=-9;
 fclose(f);
 if(error){cnk_texture_manifest_free(&m);return error;}
 *out=m;return 0;
}
int cnk_texture_manifest_scene(const cnk_texture_manifest *m,
  const cnk_scene *scene,uint32_t scene_index,const fufs_file *vfs,
  const cnk_png_catalog *catalog,cnk_texture_reference *out,
  size_t capacity,size_t *out_count){
 if(!m||!scene||!vfs||!catalog||!out_count||
    (m->count&&!m->rows)||scene_index>=vfs->count||
    (capacity&&!out)||scene->texture_count>1024)return -1;
 /* Stage only the present associations, never two 1024-element arrays on
  * the Vita main stack. Do not touch caller buffers until validation passes. */
 size_t n=0;
 for(size_t i=0;i<m->count;i++)
  if(m->rows[i].scene_vfs_index==scene_index)n++;
 if(n>scene->texture_count||n>capacity)return -2;
 if(!n){*out_count=0;return 0;}
 cnk_texture_reference *refs=calloc(n,sizeof(*refs));
 cnk_texture_binding *bindings=calloc(n,sizeof(*bindings));
 if(!refs||!bindings){free(refs);free(bindings);return -5;}
 size_t k=0;
 for(size_t i=0;i<m->count;i++){
  const cnk_manifest_row *r=&m->rows[i];
  if(r->scene_vfs_index!=scene_index)continue;
  refs[k++]=(cnk_texture_reference){r->scene_vfs_index,r->scene_resource_id,
          r->texture_slot,r->original_texture_name,r->png_vfs_index,r->png_resource_id};
 }
 int rc=cnk_texture_map_validate(scene,scene_index,vfs,catalog,refs,n,bindings,n)?-3:
        cnk_texture_map_consistency(refs,n)?-4:0;
 if(!rc){memcpy(out,refs,n*sizeof(*refs));*out_count=n;}
 free(refs);free(bindings);
 return rc;
}

int cnk_texture_manifest_verify_all(const cnk_texture_manifest *m,
                                    const fufs_file *vfs,const cnk_png_catalog *png){
 if(!m||!vfs||!png||(m->count&&!m->rows))return -1;
 /* Verify every referenced track, not only the track the player selects.
  * Refuse stale mappings before any GPU init/upload. */
 for(size_t i=0;i<m->count;i++){
  uint32_t idx=m->rows[i].scene_vfs_index;
  int previously_seen=0;
  for(size_t j=0;j<i;j++)if(m->rows[j].scene_vfs_index==idx){previously_seen=1;break;}
  if(previously_seen)continue;
  if(idx>=vfs->count)return -2;
  unsigned char *data=NULL;size_t n=0;cnk_scene scene={0};
  if(fufs_extract(vfs,idx,&data,&n))return -3;
  int parsed=cnk_parse_scene(data,n,&scene);
  free(data);
  if(parsed){cnk_scene_free(&scene);return -4;}
  size_t count=0;
  size_t cap=scene.texture_count;
  cnk_texture_reference *refs=cap<=1024u&&cap?
       calloc(cap,sizeof(*refs)):NULL;
  if(cap>1024u||(!refs&&cap)){
   cnk_scene_free(&scene);return -5;
  }
  int rc=cnk_texture_manifest_scene(m,&scene,idx,vfs,png,refs,cap,&count);
  free(refs);
  cnk_scene_free(&scene);
  if(rc||!count)return -5;
 }
 return 0;
}
