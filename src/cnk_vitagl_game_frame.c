#include "cnk_vitagl_game_frame.h"
#include "cnk_vitagl_objects.h"
#include "cnk_vitagl_items.h"
#include "cnk_item_visibility.h"
#include <string.h>
#include <limits.h>
#include <stdint.h>

static int draw_game_frame_internal(const cnk_game *game,
                              cnk_native_texture_resolve resolve,
                              void *context,
                              const cnk_racer_visual_attachment *attachments,
                              const cnk_racer_visual_attachment *drivers,
                              cnk_vitagl_frame_result *out,int cull,
                              cnk_item_sprite_resolve item_resolve,void *item_ctx,int with_items){
    if (!game || !out || !game->loaded || !game->session.active ||
        !game->render.vertices || !game->render.batches ||
        !game->render.triangle_count || !game->session.racer_count ||
        game->session.racer_count>CNK_MAX_RACERS)return -1;
    /* Check every mesh and every transform before drawing any part of the
     * frame. Unknown identity of source models never defaults to a guess. */
    float world[2][CNK_MAX_RACERS][16]={{{0}}};
    uint8_t visible[2][CNK_MAX_RACERS]={{0}};
    const cnk_racer_visual_attachment *layers[2]={attachments,drivers};
    for(unsigned layer=0;layer<2;layer++)if(layers[layer])
      for(uint32_t i=0;i<game->session.racer_count;i++){
        const cnk_racer_visual_attachment *v=&layers[layer][i];
        if(cnk_racer_visual_attachment_valid(v))return -4;
        if(v->mesh){
          if(cnk_racer_visual_matrix(&game->session.racers[i].kart,
               v->local_from_mesh,world[layer][i]))return -4;
          int query=cull?cnk_racer_visual_in_frustum(&game->session.camera,
                      v->mesh,world[layer][i]):1;
          if(query<0)return -9; /* reject BEFORE all track/GPU draws */
          /* Check every material subsection before the first GL call too.
           * No malformed original model bounds can half-render a scene. */
          if(cull)for(uint32_t j=0;j<v->mesh->batch_count;j++){
            cnk_object_mesh subset=*v->mesh;
            const cnk_object_batch *part=&v->mesh->batches[j];
            for(int axis=0;axis<3;axis++){
              subset.bbox_min[axis]=part->bbox_min[axis];
              subset.bbox_max[axis]=part->bbox_max[axis];
            }
            if(cnk_racer_visual_in_frustum(&game->session.camera,
                         &subset,world[layer][i])<0)return -9;
          }
          visible[layer][i]=(uint8_t)query;
        }
      }
    /* Validate transient items BEFORE any GPU draw call. A corrupted actor
     * must not leave the frame half-rendered. */
    cnk_item_visual_frame item_visual={0};
    if(with_items && cnk_item_visual_collect(&game->session.items,&item_visual))return -7;
    cnk_game_hud hud={0};
    if (cnk_game_hud_read(game,&hud))return -2;
    unsigned draws=0;
    uint32_t visible_tris=game->render.triangle_count;
    int rc=cull?cnk_vitagl_draw_scene_culled(&game->render,&game->session.camera,
                                            resolve,context,&draws,&visible_tris):
                cnk_vitagl_draw_scene(&game->render,&game->session.camera,
                                     resolve,context,&draws);
    if(rc)return -3;
    cnk_vitagl_frame_result result={0};
    result.race=hud;
    result.track_draw_calls=draws;
    result.visible_track_triangles=visible_tris;
    result.simulation_frames=game->simulation_frames;
    for(unsigned layer=0;layer<2;layer++)if(layers[layer])
      for(uint32_t i=0;i<game->session.racer_count;i++){
        const cnk_racer_visual_attachment *v=&layers[layer][i];
        if(!v->mesh || !visible[layer][i])continue;
        unsigned object_draws=0;
        uint32_t visible_object_triangles=v->mesh->triangle_count;
        rc=cull?cnk_vitagl_draw_object_culled(v->mesh,&game->session.camera,
             world[layer][i],v->resolve_material,v->material_context,&object_draws,
             &visible_object_triangles):
             cnk_vitagl_draw_object(v->mesh,&game->session.camera,world[layer][i],
             v->resolve_material,v->material_context,&object_draws);
        if(rc)return -5;
        unsigned *calls=layer?&result.driver_draw_calls:&result.racer_draw_calls;
        uint32_t *triangles=layer?&result.driver_triangles:&result.racer_triangles;
        unsigned *models=layer?&result.driver_models_drawn:&result.racer_models_drawn;
        if(*calls>UINT_MAX-object_draws||
           *triangles>UINT32_MAX-visible_object_triangles)return -6;
        *calls+=object_draws;
        (*models)++;
        *triangles+=visible_object_triangles;
      }
    if(with_items){
      cnk_item_visual_frame visible_items={0};
      unsigned item_calls=0,hidden=0;
      /* No GL commands for out-of-view sprites; alpha quads are sent from
       * back to front to avoid incorrect blending and needless Vita draws. */
      if(cnk_item_visibility_prepare(&item_visual,&game->session.camera,
                                     &visible_items,&hidden))return -8;
      if(cnk_vitagl_draw_items(&visible_items,&game->session.camera,
                              item_resolve,item_ctx,&item_calls))return -8;
      result.item_draw_calls=item_calls;
      result.item_actors=(uint32_t)visible_items.count;
      result.item_actors_culled=hidden;
    }
    *out=result;
    return 0;
}
int cnk_vitagl_draw_game_frame_with_racers(const cnk_game *game,
                              cnk_native_texture_resolve resolve,void *context,
                              const cnk_racer_visual_attachment *attachments,
                              cnk_vitagl_frame_result *out){
    return draw_game_frame_internal(game,resolve,context,attachments,NULL,out,0,NULL,NULL,0);
}
int cnk_vitagl_draw_game_frame_with_racers_culled(const cnk_game *game,
                              cnk_native_texture_resolve resolve,void *context,
                              const cnk_racer_visual_attachment *attachments,
                              cnk_vitagl_frame_result *out){
    return draw_game_frame_internal(game,resolve,context,attachments,NULL,out,1,NULL,NULL,0);
}
int cnk_vitagl_draw_game_frame_with_actors_culled(const cnk_game *game,
                       cnk_native_texture_resolve resolve,void *context,
                       const cnk_racer_visual_attachment *karts,
                       const cnk_racer_visual_attachment *drivers,
                       cnk_vitagl_frame_result *out){
    return draw_game_frame_internal(game,resolve,context,karts,drivers,out,1,NULL,NULL,0);
}
int cnk_vitagl_draw_game_frame(const cnk_game *game,
                              cnk_native_texture_resolve resolve,
                              void *context,
                              cnk_vitagl_frame_result *out){
    return cnk_vitagl_draw_game_frame_with_racers(game,resolve,context,NULL,out);
}

int cnk_vitagl_draw_game_frame_with_items_culled(const cnk_game *game,
           cnk_native_texture_resolve resolve,void *context,
           const cnk_racer_visual_attachment *karts,
           const cnk_racer_visual_attachment *drivers,
           cnk_item_sprite_resolve item_resolve,void *item_context,
           cnk_vitagl_frame_result *out){
 return draw_game_frame_internal(game,resolve,context,karts,drivers,out,1,
                                 item_resolve,item_context,1);
}
