#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#define LIMIT (128u*1024u*1024u)
#define RESOURCE_LIMIT 50000u
static uint32_t u32(const unsigned char *b) { return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24); }
static int blob_copy(const unsigned char *in, size_t n, unsigned char **out, size_t *size) {
  unsigned char *b=(unsigned char *)malloc(n?n:1);
  if(!b) return -1;
  if(n) memcpy(b,in,n);
  *out=b; *size=n; return 0;
}
void fufs_close(fufs_file *f) {if(f) {free(f->data);free(f->entries);memset(f,0,sizeof(*f));}}
int fufs_open(const char *path, fufs_file *out) {
  FILE *h; long l; size_t len; unsigned char *b; uint32_t count,i;
  if(!path||!out) return -1;
  memset(out,0,sizeof(*out));
  h=fopen(path,"rb"); if(!h) return -2;
  if(fseek(h,0,SEEK_END)|| (l=ftell(h))<12 || l>64*1024*1024L || fseek(h,0,SEEK_SET)) {fclose(h);return -3;}
  len=(size_t)l; b=(unsigned char*)malloc(len);
  if(!b){fclose(h);return -4;}
  if(fread(b,1,len,h)!=len){free(b);fclose(h);return -5;}
  fclose(h);
  if(memcmp(b,"FUFS",4)){free(b);return -6;}
  count=u32(b+8);
  if(count>RESOURCE_LIMIT||12+(size_t)count*12>len){free(b);return -7;}
  out->entries=(fufs_entry *)calloc(count?count:1,sizeof(fufs_entry));
  if(!out->entries){free(b);return -8;}
  for(i=0;i<count;i++){
    const unsigned char *e=b+12+i*12;
    uint32_t off=u32(e), sz=u32(e+8);
    if(off<12+(size_t)count*12 || (size_t)off>len || (size_t)sz>len-off){free(b);free(out->entries);memset(out,0,sizeof(*out));return -9;}
    out->entries[i].offset=off;out->entries[i].id=u32(e+4);out->entries[i].size=sz;
  }
  out->data=b;out->length=len;out->count=count;return 0;
}
static int is_zlib(const unsigned char *p,size_t n){return n>1&&p[0]==0x78&&(p[1]==0x01||p[1]==0x9c||p[1]==0xda);}
static int inflate_limited(const unsigned char *in,size_t n,unsigned char **out,size_t *olen){
  z_stream z; unsigned char *buf; size_t cap=65536; int status;
  if(n>0xffffffffu)return -1;
  buf=(unsigned char*)malloc(cap);if(!buf)return -2;
  memset(&z,0,sizeof(z));
  if(inflateInit(&z)!=Z_OK){free(buf);return -3;}
  z.next_in=(Bytef*)in;z.avail_in=(uInt)n;
  do {
    if(z.total_out==cap){unsigned char *p;if(cap>=LIMIT){status=Z_MEM_ERROR;break;}cap=cap*2>LIMIT?LIMIT:cap*2;p=(unsigned char*)realloc(buf,cap);if(!p){status=Z_MEM_ERROR;break;}buf=p;}
    z.next_out=buf+z.total_out;z.avail_out=(uInt)(cap-z.total_out);
    status=inflate(&z,Z_NO_FLUSH);
    if(status==Z_OK&&z.avail_in==0&&z.avail_out>0){status=Z_DATA_ERROR;break;}
  }while(status==Z_OK);
  if(status!=Z_STREAM_END){inflateEnd(&z);free(buf);return -4;}
  *olen=z.total_out;*out=buf;inflateEnd(&z);return 0;
}
int fufs_extract(const fufs_file *f,uint32_t index,unsigned char **out,size_t *size){
  const unsigned char *p;size_t n;int ret,k;unsigned char *buf=NULL;size_t blen=0;
  if(!f||!f->data||!out||!size||index>=f->count)return -1;
  *out=NULL;*size=0;
  p=f->data+f->entries[index].offset;n=f->entries[index].size;
  if(n>=12&&!memcmp(p,"PLZP",4))ret=inflate_limited(p+12,n-12,&buf,&blen);
  else if(is_zlib(p,n))ret=inflate_limited(p,n,&buf,&blen);
  else ret=blob_copy(p,n,&buf,&blen);
  if(ret)return ret;
  for(k=0;k<3&&is_zlib(buf,blen);k++){
    unsigned char *nested=NULL;size_t nested_len=0;
    if(inflate_limited(buf,blen,&nested,&nested_len))break;
    free(buf);buf=nested;blen=nested_len;
  }
  *out=buf;*size=blen;return 0;
}
const char *fufs_type(const unsigned char *d,size_t n){
  if(n>=8&&!memcmp(d,"\x89PNG\r\n\x1a\n",8))return "png";
  if(n>=12&&!memcmp(d,"RIFF",4)&&!memcmp(d+8,"WAVE",4))return "wav";
  if(n>=3&&!memcmp(d,"ID3",3))return "mp3";
  if(n>=2&&d[0]==0xff&&(d[1]==0xfb||d[1]==0xf3))return "mp3";
  if(n>=4&&!memcmp(d,"PIFF",4))return "piff";
  if(n>=4&&!memcmp(d,"PFNT",4))return "pfnt";
  if(n>=4&&!memcmp(d,"LANG",4))return "lang";
  if(n>=4&&!memcmp(d,"FORM",4))return "form";
  return "bin";
}
