#include "cnk_frustum.h"
#include <math.h>

int cnk_batch_in_frustum(const cnk_camera *cam,const cnk_render_batch *batch){
 if(!cam||!batch)return -1;
 for(int i=0;i<16;i++)if(!isfinite(cam->view_projection[i]))return -1;
 for(int i=0;i<3;i++){
  if(!isfinite(batch->bbox_min[i])||!isfinite(batch->bbox_max[i])||
      batch->bbox_min[i]>batch->bbox_max[i])return -1;
 }
 /* Column-major VP, six homogeneous clip planes: w +/- x,y,z >= 0.
  * A box is invisible ONLY if all eight corners lie behind a plane.
  * Double precision + a small tolerance prevent discarding boundary
  * geometry due to multiplication rounding at high source coordinates. */
 const float *p=cam->view_projection;
 for(int axis=0;axis<3;axis++)for(int sign=-1;sign<=1;sign+=2){
  double plane[4];
  for(int c=0;c<4;c++)plane[c]=(double)p[c*4+3]+sign*(double)p[c*4+axis];
  double largest=plane[3];
  double tolerance=1e-5*(fabs(plane[3])+1.0);
  for(int c=0;c<3;c++){
   const double a=(double)batch->bbox_min[c],b=(double)batch->bbox_max[c];
   const double va=plane[c]*a,vb=plane[c]*b;
   largest+=va>vb?va:vb;
   tolerance+=1e-5*fmax(fabs(va),fabs(vb));
  }
  if(largest < -tolerance)return 0;
 }
 return 1;
}
