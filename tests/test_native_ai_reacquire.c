/* No ROM required. Reproduces a rival displaced so far along a source-style
 * DRV route that every continuity-constrained segment is rejected.
 * A failed cnk_ai_controls used to propagate as a fatal session tick error. */
#include "cnk_drive.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void){
 cnk_vec3 points[]={{0,0,0},{100,0,0},{100,0,100},{0,0,100}};
 cnk_sline route={0};
 route.count=4;
 route.points=points;
 cnk_drive_tuning tuning={0};
 cnk_drive_default_tuning(&tuning);
 cnk_ai_driver ai={0};
 assert(cnk_ai_init(&ai,&route,1,&tuning)==0);
 assert(fabsf(ai.route_length-400.0f)<0.001f);

 cnk_kart kart={0};
 assert(cnk_kart_spawn(&kart,(cnk_vec3){100,0,50},0.0f)==0);
 cnk_driver_input controls={0};
 /* Old local window is near the first straight: none of the four
  * candidate projections is within its allowed progress interval. */
 ai.progress_initialized=1;
 ai.progress_units=20.0f;
 assert(cnk_ai_controls(&ai,&kart,&tuning,&controls)==0);
 assert(fabsf(ai.route_fraction-0.375f)<0.001f);
 assert(fabsf(ai.progress_units-150.0f)<0.001f);
 assert(ai.route_distance<0.001f);
 assert(isfinite(controls.steer) && fabsf(controls.steer)<=1.0f);
 assert(isfinite(controls.throttle) && isfinite(controls.brake));

 /* Same relocation on a subsequent loop: keep progress unwrapped.
  * Guidance must continue normally on the very next frame. */
 ai.progress_units=420.0f;
 ai.progress_initialized=1;
 assert(cnk_ai_controls(&ai,&kart,&tuning,&controls)==0);
 assert(fabsf(ai.progress_units-550.0f)<0.001f);
 assert(cnk_ai_controls(&ai,&kart,&tuning,&controls)==0);
 assert(fabsf(ai.progress_units-550.0f)<0.001f);

 /* The existing large-lateral-error recovery path must also retain the
  * lap index instead of incorrectly resetting to the first 400 units. */
 ai.progress_units=450.0f;
 ai.progress_initialized=1;
 kart.position=(cnk_vec3){100,0,50};
 assert(cnk_ai_controls(&ai,&kart,&tuning,&controls)==0);
 assert(fabsf(ai.progress_units-550.0f)<0.001f);
 puts("PASS: displaced rival reacquires source route without fatal tick");
 return 0;
}
