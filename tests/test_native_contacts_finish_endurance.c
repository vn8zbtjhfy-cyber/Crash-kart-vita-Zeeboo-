/* Regression for development-only kart contacts and completed-racer cleanup.
 * These collisions and the opponent grid are NOT recovered Zeebo mechanics.
 * Verifies 72 real checkpoint/lap completions over the 12 VFS race scenes
 * while the optional contact system is enabled, without manufactured laps. */
#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void active_racers_still_collide(fufs_file *source){
 cnk_game game={0};
 assert(cnk_game_load_original_start(&game,source,37,2,1)==0);
 assert(cnk_game_enable_provisional_kart_contacts(&game,1)==0);
 game.session.racers[1].kart.position=game.session.racers[0].kart.position;
 assert(cnk_game_update(&game,(cnk_driver_input){1.0f,0,0},16667)==1);
 const cnk_session_racer *a=&game.session.racers[0],*b=&game.session.racers[1];
 float distance=hypotf(a->kart.position.x-b->kart.position.x,
                       a->kart.position.z-b->kart.position.z);
 assert(a->last_kart_contact==1 && b->last_kart_contact==1);
 assert(a->kart_contact_count==1 && b->kart_contact_count==1);
 assert(distance>=game.session.contact_tuning.radius*2-0.0001f);
 assert(a->race.completed_laps==0 && b->race.completed_laps==0);
 assert(!a->race.finished && !b->race.finished);
 cnk_game_unload(&game);
}

static void finished_racer_is_not_a_finish_line_wall(fufs_file *source){
 cnk_game with_contacts={0},without_contacts={0};
 assert(cnk_game_load_original_start(&with_contacts,source,37,2,1)==0);
 assert(cnk_game_load_original_start(&without_contacts,source,37,2,1)==0);
 /* Force a FINISHED opponent onto the human kart's launch position.  This
  * isolates the finished-kart filter from navigation and overtaking AI. */
 for(int j=0;j<2;j++){
  cnk_game *game=j==0?&with_contacts:&without_contacts;
  cnk_session_racer *opponent=&game->session.racers[1];
  opponent->race.finished=1;
  opponent->kart.position=game->session.racers[0].kart.position;
  opponent->kart.speed=0;
  opponent->finish_ms=1;
 }
 assert(cnk_game_enable_provisional_kart_contacts(&with_contacts,1)==0);
 cnk_vec3 original=with_contacts.session.racers[1].kart.position;
 for(unsigned frame=0;frame<120;frame++){
  cnk_driver_input forward={1.0f,0,0};
  assert(cnk_game_update(&with_contacts,forward,16667)==1);
  assert(cnk_game_update(&without_contacts,forward,16667)==1);
  const cnk_session_racer *a=&with_contacts.session.racers[0];
  const cnk_session_racer *b=&without_contacts.session.racers[0];
  assert(!a->last_kart_contact);
  assert(a->kart_contact_count==0);
  assert(fabsf(a->kart.position.x-b->kart.position.x)<0.0001f);
  assert(fabsf(a->kart.position.z-b->kart.position.z)<0.0001f);
  assert(a->race.completed_laps==b->race.completed_laps);
  assert(a->race.next_gate==b->race.next_gate);
  assert(memcmp(&with_contacts.session.racers[1].kart.position,&original,sizeof(original))==0);
  assert(!with_contacts.session.racers[1].last_kart_contact);
 }
 assert(hypotf(with_contacts.session.racers[0].kart.position.x-original.x,
               with_contacts.session.racers[0].kart.position.z-original.z)>0.3f);
 cnk_game_unload(&with_contacts);
 cnk_game_unload(&without_contacts);
}

int main(int argc,char **argv){
 assert(argc==2 || (argc==3 && strcmp(argv[2],"--quick")==0));
 fufs_file source={0};assert(fufs_open(argv[1],&source)==0);
 active_racers_still_collide(&source);
 finished_racer_is_not_a_finish_line_wall(&source);
 if(argc==3){
  fufs_close(&source);
  puts("PASS: focused finished-racer barrier and gate safety check");
  return 0;
 }
 const unsigned scenes[12]={37,39,42,69,73,77,207,211,213,295,298,300};
 unsigned finished_total=0;
 for(unsigned track=0;track<12;track++){
  cnk_game game={0};
  assert(cnk_game_load_original_start(&game,&source,scenes[track],6,1)==0);
  assert(cnk_game_enable_provisional_kart_contacts(&game,1)==0);
  unsigned frame;
  for(frame=0;frame<26000;frame++){
   cnk_driver_input human={0};
   if(!game.session.racers[0].race.finished)
    assert(cnk_ai_controls(&game.session.racers[0].ai,
          &game.session.racers[0].kart,&game.session.tuning,&human)==0);
   assert(cnk_game_update(&game,human,16667)==1);
   unsigned finished=0;
   for(unsigned k=0;k<6;k++){
    const cnk_session_racer *r=&game.session.racers[k];
    assert(isfinite(r->kart.position.x)&&isfinite(r->kart.position.y));
    assert(isfinite(r->kart.position.z)&&isfinite(r->kart.speed));
    assert(r->race.completed_laps<=1);
    if(r->race.finished){
     assert(r->race.completed_laps==1);
     finished++;
    }
   }
   if(finished==6)break;
  }
  assert(frame<26000);
  cnk_game_classification result={0};
  assert(cnk_game_classification_read(&game,&result)==0);
  assert(result.count==6);
  for(unsigned k=0;k<6;k++){
   assert(result.rows[k].finished);
   assert(result.rows[k].completed_laps==1);
   assert(result.rows[k].rank==k+1);
   finished_total++;
  }
  printf("Track %u: 6 of 6 finish with experimental contacts enabled (%u frames)\n",scenes[track],frame+1);
  cnk_game_unload(&game);
 }
 fufs_close(&source);
 assert(finished_total==72);
 puts("PASS: 72/72 finishes and no finished-kart ghost barrier (PC simulated physics)");
 return 0;
}
