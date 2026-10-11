#ifndef CNK_CAMERA_H
#define CNK_CAMERA_H
#include "cnk_scene.h"
#include "cnk_drive.h"
/* OpenGL column-major matrices in original Zeebo world coordinates. */
typedef struct {float view[16],projection[16],view_projection[16];cnk_vec3 eye,target;} cnk_camera;
int cnk_camera_build(cnk_camera *out,cnk_vec3 eye,cnk_vec3 target,
                     float vfov_rad,float aspect,float near_plane,float far_plane);
int cnk_camera_follow(cnk_camera *out,const cnk_kart *kart,
                      float distance,float lift,float lookahead,
                      float vfov_rad,float aspect);
/* Occlusion-aware *provisional* native follow camera. Cast from an elevated
 * player anchor to the proposed camera eye, against original scene triangles.
 * A hit pulls the eye in front of the surface by `margin` world units.
 * On miss/NULL grid behavior matches cnk_camera_follow exactly.
 * This is a point-camera occlusion query, NOT original Polarbit camera logic;
 * unavailable scene triangles are not silently replaced with fake geometry.
 * Returns 0 on success; leaves `out` untouched on error. */
int cnk_camera_follow_obstructed(cnk_camera *out,const cnk_kart *kart,
                      const cnk_collision_grid *world,
                      float distance,float lift,float lookahead,
                      float vfov_rad,float aspect,float margin);
/* Clip-space projection for host-test and HUD anchors; 0 inside, -2 outside. */
int cnk_camera_project(const cnk_camera *cam,cnk_vec3 point,float *screen_x,float *screen_y);
#endif
