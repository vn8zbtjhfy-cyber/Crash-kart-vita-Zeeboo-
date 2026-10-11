/* Testing stand-in collision discs, NOT source-authentic Zeebo kart physics. */
#include "cnk_kart_contact.h"
#include "cnk_game.h"
#include "fufs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static cnk_vec3 p(float x,float y,float z){return (cnk_vec3){x,y,z};}
static float distance(cnk_kart a,cnk_kart b){
 return hypotf(a.position.x-b.position.x,a.position.z-b.position.z);
}
static void pair_tests(void){
 cnk_kart_contact_tuning cfg;cnk_kart_contact_default(&cfg);
 assert(cnk_kart_contact_validate(&cfg)==0);
 cnk_kart_contact_tuning bad=cfg;bad.radius=NAN;
 assert(cnk_kart_contact_validate(&bad)<0);
 bad=cfg;bad.height_tolerance=-1;
 assert(cnk_kart_contact_validate(&bad)<0);
 cnk_kart k[2]={0};cnk_vec3 before[2]={p(-1,0,0),p(1,0,0)};
 uint8_t active[2]={1,1},contacts[2]={9,9};
 /* Each centre crosses the other's location within one frame, even though
  * their final positions are once again far apart. Sweep prevents tunneling. */
 assert(cnk_kart_spawn(&k[0],p(1,0,0),1.57079f)==0);
 assert(cnk_kart_spawn(&k[1],p(-1,0,0),-1.57079f)==0);
 k[0].speed=10;k[1].speed=10;
 assert(cnk_kart_contacts_resolve(k,before,active,2,&cfg,contacts)==1);
 assert(contacts[0]==1&&contacts[1]==1);
 assert(k[0].position.x<0&&k[1].position.x>0);
 assert(distance(k[0],k[1])>=cfg.radius*2-0.0001f);
 assert(k[0].speed==0&&k[1].speed==0);
 /* Heights differ, as on an overpass: no ghost collision. */
 before[1]=p(1,5,0);
 k[0].position=p(1,0,0);k[1].position=p(-1,5,0);
 assert(cnk_kart_contacts_resolve(k,before,active,2,&cfg,contacts)==0);
 assert(!contacts[0]&&!contacts[1]);
 /* Parallel travel without approaching. */
 before[0]=p(-1,0,0);before[1]=p(1,0,0);
 k[0].position=p(-1,0,1);k[1].position=p(1,0,1);
 assert(cnk_kart_contacts_resolve(k,before,active,2,&cfg,contacts)==0);
 /* Initial overlap must be separated. Opponent is frozen at finish. */
 before[0]=p(0,0,0);before[1]=p(0.1f,0,0);
 k[0].position=before[0];k[1].position=before[1];
 active[0]=1;active[1]=0;
 assert(cnk_kart_contacts_resolve(k,before,active,2,&cfg,contacts)==1);
 assert(distance(k[0],k[1])>=cfg.radius*2-0.0001f);
 assert(fabsf(k[1].position.x-0.1f)<1.e-6f);
 /* Early impact prevents phantom contact with a racer far beyond it. */
 cnk_kart chain[3]={0};
 cnk_vec3 chain_before[3]={p(-2,0,0),p(-1,0,0),p(1,0,0)};
 uint8_t chain_movable[3]={1,0,0},chain_flags[3]={7,7,7};
 assert(cnk_kart_spawn(&chain[0],p(2,0,0),1.57079f)==0);
 assert(cnk_kart_spawn(&chain[1],chain_before[1],0)==0);
 assert(cnk_kart_spawn(&chain[2],chain_before[2],0)==0);
 assert(cnk_kart_contacts_resolve(chain,chain_before,chain_movable,3,&cfg,chain_flags)==1);
 assert(chain_flags[0]&&chain_flags[1]&&!chain_flags[2]);
 assert(chain[0].position.x< -1.2f && chain[2].position.x==1.0f);
 /* Ordering the far rival before the near rival must not change the event. */
 cnk_kart reordered[3]={chain[0],chain[2],chain[1]};
 reordered[0].position=p(2,0,0);reordered[0].speed=4;
 cnk_vec3 reorder_before[3]={chain_before[0],chain_before[2],chain_before[1]};
 uint8_t reorder_movable[3]={1,0,0},reorder_flags[3]={0};
 assert(cnk_kart_contacts_resolve(reordered,reorder_before,reorder_movable,3,&cfg,reorder_flags)==1);
 assert(reorder_flags[0]&&!reorder_flags[1]&&reorder_flags[2]);
 assert(reordered[0].position.x< -1.2f);
 /* A bad input cannot alter positions or caller flags. */
 cnk_kart saved[2];memcpy(saved,k,sizeof(k));
 contacts[0]=9;contacts[1]=9;
 assert(cnk_kart_contacts_resolve(k,before,active,2,&bad,contacts)<0);
 assert(memcmp(k,saved,sizeof(k))==0&&contacts[0]==9&&contacts[1]==9);
}
static void scene_tests(const char *path){
 fufs_file vf={0};assert(fufs_open(path,&vf)==0);
 unsigned tested=0,rejected=0;
 for(uint32_t i=0;i<vf.count;i++){
  unsigned char *b=NULL;size_t n=0;assert(fufs_extract(&vf,i,&b,&n)==0);
  int is_scene=n>=12&&!memcmp(b,"PIFF",4)&&!memcmp(b+8,"SCEN",4);
  free(b);if(!is_scene)continue;
  cnk_game g={0};int rc=cnk_game_load_original_start(&g,&vf,i,3,2);
  if(rc){assert(rc==-8);rejected++;continue;}
  assert(!g.session.provisional_kart_contacts_enabled);
  assert(cnk_game_enable_provisional_kart_contacts(&g,1)==0);
  for(unsigned t=0;t<30;t++){
   assert(cnk_game_update(&g,(cnk_driver_input){1,0,0},16667)==1);
   for(unsigned j=0;j<3;j++){
    const cnk_session_racer *r=&g.session.racers[j];
    assert(isfinite(r->kart.position.x)&&isfinite(r->kart.position.y)&&isfinite(r->kart.position.z));
    assert(r->last_kart_contact==0||r->last_kart_contact==1);
    assert(r->kart_contact_count<=30);
   }
  }
  assert(cnk_game_enable_provisional_kart_contacts(&g,0)==0);
  assert(cnk_game_update(&g,(cnk_driver_input){1,0,0},16667)==1);
  for(unsigned j=0;j<3;j++)assert(g.session.racers[j].last_kart_contact==0);
  cnk_game_unload(&g);tested++;
 }
 assert(tested==12&&rejected==2);
 fufs_close(&vf);
 printf("PASS: provisional kart contacts on %u original racing scenes, %u special scenes skipped. NOT source physics.\n",tested,rejected);
}
int main(int argc,char **argv){assert(argc==2);pair_tests();scene_tests(argv[1]);return 0;}
