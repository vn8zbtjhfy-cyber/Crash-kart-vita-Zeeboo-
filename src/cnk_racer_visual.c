#include "cnk_racer_visual.h"
#include <math.h>
#include <limits.h>
#include <stdint.h>

int cnk_racer_visual_matrix(const cnk_kart *kart,
                            const float local_from_mesh[16],
                            float out_world_from_mesh[16]){
    if(!kart || !local_from_mesh || !out_world_from_mesh ||
       !isfinite(kart->position.x) || !isfinite(kart->position.y) ||
       !isfinite(kart->position.z) || !isfinite(kart->heading))return -1;
    for(unsigned i=0;i<16;i++)if(!isfinite(local_from_mesh[i]))return -2;
    /* Only accept an affine source-model calibration, not a projection. */
    if(fabsf(local_from_mesh[3])>1e-6f ||
       fabsf(local_from_mesh[7])>1e-6f ||
       fabsf(local_from_mesh[11])>1e-6f ||
       fabsf(local_from_mesh[15]-1.0f)>1e-6f)return -3;
    float s=sinf(kart->heading),c=cosf(kart->heading);
    if(!isfinite(s)||!isfinite(c))return -4;
    /* T(position)*Ry(heading), OpenGL column-major. */
    const float base[16]={c,0,-s,0, 0,1,0,0, s,0,c,0,
                           kart->position.x,kart->position.y,kart->position.z,1};
    float result[16];
    for(int col=0;col<4;col++)for(int row=0;row<4;row++){
        float sum=0;
        for(int k=0;k<4;k++)sum+=base[k*4+row]*local_from_mesh[col*4+k];
        if(!isfinite(sum))return -5;
        result[col*4+row]=sum;
    }
    for(int i=0;i<16;i++)out_world_from_mesh[i]=result[i];
    return 0;
}

int cnk_racer_visual_attachment_valid(const cnk_racer_visual_attachment *v){
    if(!v)return -1;
    if(!v->mesh)return (v->local_from_mesh||v->resolve_material||v->material_context)?-2:0;
    if(!v->local_from_mesh)return -3;
    const cnk_object_mesh *m=v->mesh;
    if(!m->vertices||!m->batches||!m->vertex_count||
       !m->batch_count||m->vertex_count%3u||
       m->triangle_count!=m->vertex_count/3u)return -4;
    uint32_t cursor=0;
    for(uint32_t i=0;i<m->batch_count;i++){
        const cnk_object_batch *b=&m->batches[i];
        if(b->first_vertex!=cursor||b->vertex_count%3u||
           b->vertex_count>m->vertex_count-cursor||cursor>INT_MAX||
           b->vertex_count>INT_MAX)return -5;
        cursor+=b->vertex_count;
    }
    if(cursor!=m->vertex_count)return -6;
    for(unsigned i=0;i<m->vertex_count;i++){
        const cnk_render_vertex *p=&m->vertices[i];
        if(!isfinite(p->x)||!isfinite(p->y)||!isfinite(p->z)||
           !isfinite(p->u)||!isfinite(p->v))return -7;
    }
    for(int i=0;i<16;i++)if(!isfinite(v->local_from_mesh[i]))return -8;
    if(fabsf(v->local_from_mesh[3])>1e-6f||
       fabsf(v->local_from_mesh[7])>1e-6f||
       fabsf(v->local_from_mesh[11])>1e-6f||
       fabsf(v->local_from_mesh[15]-1)>1e-6f)return -9;
    return 0;
}

/* Transform the actual source 3DOB bounds into a conservative world-space
 * box; then reject only if all of it lies behind a homogeneous clip plane.
 * Double precision limits rounding on large coordinates in Zeebo scenes. */
int cnk_racer_visual_in_frustum(const cnk_camera *cam,
                   const cnk_object_mesh *mesh,const float world[16]){
    if(!cam||!mesh||!world)return -1;
    for(int i=0;i<16;i++)if(!isfinite(world[i])||
       !isfinite(cam->view_projection[i]))return -2;
    if(fabsf(world[3])>1e-6f||fabsf(world[7])>1e-6f||
       fabsf(world[11])>1e-6f||fabsf(world[15]-1.f)>1e-6f)return -3;
    for(int i=0;i<3;i++)if(!isfinite(mesh->bbox_min[i])||
       !isfinite(mesh->bbox_max[i])||mesh->bbox_min[i]>mesh->bbox_max[i])return -4;
    double mid[3],extent[3],world_mid[3],world_half[3];
    for(int i=0;i<3;i++){
      mid[i]=((double)mesh->bbox_min[i]+mesh->bbox_max[i])*0.5;
      extent[i]=((double)mesh->bbox_max[i]-mesh->bbox_min[i])*0.5;
    }
    for(int r=0;r<3;r++){
      world_mid[r]=world[12+r];world_half[r]=0;
      for(int c=0;c<3;c++){
        double v=world[c*4+r];
        world_mid[r]+=v*mid[c];
        world_half[r]+=fabs(v)*extent[c];
      }
      if(!isfinite(world_mid[r])||!isfinite(world_half[r]))return -5;
    }
    const float *p=cam->view_projection;
    for(int axis=0;axis<3;axis++)for(int sign=-1;sign<=1;sign+=2){
      double farthest=(double)p[15]+sign*(double)p[12+axis];
      double tolerance=1e-5*(fabs(farthest)+1.0);
      for(int i=0;i<3;i++){
        double coeff=(double)p[i*4+3]+sign*(double)p[i*4+axis];
        double val=coeff*world_mid[i];
        double extent_part=fabs(coeff)*world_half[i];
        farthest+=val+extent_part;
        tolerance+=1e-5*(fabs(val)+extent_part);
      }
      if(farthest < -tolerance)return 0;
    }
    return 1;
}
