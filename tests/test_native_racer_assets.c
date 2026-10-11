#define _POSIX_C_SOURCE 200809L
#include "cnk_racer_assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int find_model(const fufs_file *vfs){
 for(unsigned i=0;i<vfs->count;i++){
  unsigned char *b=NULL;size_t n=0;
  assert(!fufs_extract(vfs,i,&b,&n));
  int yes=n>=12&&!memcmp(b,"PIFF",4)&&!memcmp(b+8,"3DOB",4);
  free(b);
  if(yes)return (int)i;
 }
 return -1;
}
static void one_row(FILE *f,unsigned slot,unsigned index,unsigned id,const char *matrix){
 assert(fprintf(f,"%u\t%u\t%08x\t%s\n",slot,index,id,matrix)>0);
}
static const char *valid_matrix="1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1";
static const char *bad_matrix="1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t0\t0\t0\t0\t0\t1";
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 int idx=find_model(&vfs);assert(idx>=0);
 unsigned rid=vfs.entries[idx].id;
 char path[]="/tmp/cnk_racer_assets_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);
 FILE *f=fdopen(fd,"wb");assert(f);
 fprintf(f,"# TEST ONLY; not actual character bindings\n");
 one_row(f,0,(unsigned)idx,rid,valid_matrix);
 one_row(f,2,(unsigned)idx,rid,valid_matrix);
 assert(!fclose(f));
 cnk_racer_assets assets={0};
 assert(!cnk_racer_assets_load(path,&vfs,&assets));
 assert(assets.assigned[0]&&assets.assigned[2]&&!assets.assigned[1]);
 assert(assets.resource_id[0]==rid&&assets.index[2]==(unsigned)idx);
 cnk_racer_visual_attachment visuals[CNK_MAX_RACERS];
 cnk_racer_assets_attachments(&assets,visuals);
 assert(visuals[0].mesh&&visuals[2].mesh&&!visuals[1].mesh);
 assert(!cnk_racer_visual_attachment_valid(&visuals[0]));
 assert(visuals[0].mesh->triangle_count>0);
 cnk_racer_assets_free(&assets);
 cnk_racer_assets_attachments(&assets,visuals);
 assert(!visuals[0].mesh);
 f=fopen(path,"wb");assert(f);
 one_row(f,0,(unsigned)idx,rid,valid_matrix);
 one_row(f,0,(unsigned)idx,rid,valid_matrix);
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);
 assert(!assets.assigned[0]);
 f=fopen(path,"wb");assert(f);
 one_row(f,1,(unsigned)idx,rid^1u,valid_matrix);
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);
 f=fopen(path,"wb");assert(f);
 one_row(f,1,(unsigned)idx,rid,bad_matrix);
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);
 f=fopen(path,"wb");assert(f);
 one_row(f,1,(unsigned)idx,rid,valid_matrix);
 one_row(f,2,(unsigned)idx,rid,bad_matrix);
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);
 assert(!assets.assigned[1]&&!assets.assigned[2]); /* rollback */
 f=fopen(path,"wb");assert(f);
 one_row(f,1,(unsigned)idx,rid,"nan\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1");
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);
 assert(!remove(path));
 assert(cnk_racer_assets_load(path,&vfs,&assets)==-2);
 fufs_close(&vfs);
 puts("PASS: original 3DOB source-ID + per-racer affine assignments verified; malformed/duplicate/unverified assets rejected atomically. No kart identity inferred.");
 return 0;
}
