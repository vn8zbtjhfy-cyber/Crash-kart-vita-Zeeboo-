//! Vita-native Zeebx FFI, experimental and not yet verified on console.
mod fufs;
use std::{cell::UnsafeCell,ffi::{CStr,CString,c_char},ptr,slice};
use zeebx::{cpu::interpretador::Interpretador,input::Pad,loader::{self,miffile::MifFile,modfile::ModImage},machine::{Machine,AppletResult,Outcome},storage::StoragePaths};
use fufs::FufsView;

// Every log call closes the file on the C side, surviving panic=abort.
// No Rust allocations are required to log constant boot-stage strings.
unsafe extern "C" {
 fn zeebx_vita_debug_log(stage: *const c_char);
}
fn log_stage(stage: &CStr) {
 unsafe { zeebx_vita_debug_log(stage.as_ptr()) }
}
#[repr(C)]#[derive(Clone,Copy)]pub struct EmbeddedAsset{pub bytes:*const u8,pub size:usize}
type EmbeddedLookup=unsafe extern "C" fn(*const c_char)->EmbeddedAsset;
#[repr(C)]#[derive(Clone,Copy)]pub struct ZeebxVitaPad{pub buttons:u32,pub axes:[i32;4]}
const MOD:&CStr=c"brew/mod/274214/cnk2.mod";
const VFS:&CStr=c"brew/mod/274214/data.vfs";
const MIF:&CStr=c"brew/mif/274214.mif";
const BOOT_BUDGET:u64=20_000_000;
// Short bounded slices yield control to the Vita UI rather than executing
// hundreds of millions of interpreted guest instructions in a single frame.
// Lower bounded guest-execution slices let the UI poll Vita buttons often.
// An interpreted callback may run to the entire budget before returning;
// 1M instructions x 3 guest pipelines x 4 passes caused multi-second stalls.
// Slice more often rather than reducing game logic, textures, or physics.
// Smaller bounded slices improve button/exit responsiveness on a 333 MHz Vita.
// This caps per-call interpreter latency, not total guest work.
const FRAME_BUDGET:u64=64_000;
const FRAME_PASSES:usize=2;
// CreateInstance is no longer a 24*20M-instruction blocking call.
const CREATE_SLICE:u64=125_000;
const MAX_CREATE_POLLS:u32=2048;
struct Running{
 machine:Machine<Interpretador>,applet:Option<(u32,u32)>,pad_before:Pad,
 clsid:u32,creation_started:bool,creation_finished:bool,creation_polls:u32,
 frame:Vec<u8>,frame565:Vec<u16>,w:u32,h:u32,logged_first_tick:bool,ticks:u32,logged_video:bool,logged_nonblank:bool,post_start_trace:u8,next_report_ms:u32,menu_capture_saved:bool,race_capture_saved:bool,capture_enabled:bool,autotest_enabled:bool,next_scene_validation_ms:u32,verified_menu:bool,verified_race:bool,_fufs:FufsView
}
struct State{running:Option<Running>,error:Option<CString>}
struct StateCell(UnsafeCell<State>);
unsafe impl Sync for StateCell{} // C entrypoints called on Vita main thread only
static STATE:StateCell=StateCell(UnsafeCell::new(State{running:None,error:None}));
fn global()->&'static mut State{unsafe{&mut*STATE.0.get()}}
fn fail(e:impl AsRef<str>)->i32{
 global().error=CString::new(e.as_ref().replace('\0'," ")).ok();-1
}
unsafe fn asset(lookup:EmbeddedLookup,name:&CStr)->Result<&'static[u8],String>{
 let a=unsafe{lookup(name.as_ptr())};
 if a.bytes.is_null()||a.size==0||a.size>32*1024*1024{return Err(format!("Recurso faltante: {}",name.to_string_lossy()));}
 Ok(unsafe{slice::from_raw_parts(a.bytes,a.size)})
}
fn boot(lookup:EmbeddedLookup)->Result<Running,String>{
 // panic=abort yields Vita's generic C2 crash. Persist Rust panic message
 // and exact source location BEFORE the process aborts.
 std::panic::set_hook(Box::new(|info| {
  if let Ok(message)=CString::new(format!("R.PANIC: {info}")){
   log_stage(&message);
  } else { log_stage(c"R.PANIC.unprintable"); }
 }));
 log_stage(c"R.boot.start");
 let modbytes=unsafe{asset(lookup,MOD)}?;
 let mif=unsafe{asset(lookup,MIF)}?;
 let fufs=FufsView::parse(unsafe{asset(lookup,VFS)}?)?;
 log_stage(c"R.boot.assets.fufs.ok");
 let clsid=MifFile::parse(mif).map_err(|e|e.to_string())?.main_applet().ok_or("MIF sin applet")?;
 log_stage(c"R.boot.mif.ok");
 let modimage=ModImage::parse(modbytes.to_vec()).map_err(|e|e.to_string())?;
 let loaded=loader::load_with(&modimage,&[]).map_err(|e|e.to_string())?;
 log_stage(c"R.boot.mod.loaded");
 log_stage(c"R.boot.cpu.enter");
 let cpu=Interpretador::new().map_err(|e|e.to_string())?;
 log_stage(c"R.boot.cpu.ok");
 let storage=StoragePaths::from_root("ux0:data/CNKVITA01");
 // Machine::new_with_storage configures paths but does not create them.
 // FileMgr::OpenFile(OFM_CREATE) does not create parent directories.
 // The original game reads/writes udata/settings.dat and saved progress,
 // so prepare the persistent guest save tree before any BREW callbacks.
 storage.create_dirs().map_err(|e|format!("No se pudo inicializar el almacenamiento del juego: {e}"))?;
 std::fs::create_dir_all(storage.saves.join("cnk3d/udata"))
  .map_err(|e|format!("No se pudo crear la carpeta de ajustes del juego: {e}"))?;
 log_stage(c"R.boot.storage.ready");
 log_stage(c"R.boot.machine.enter");
 let mut machine=Machine::new_with_storage(cpu,loaded,"app0:/virtual-mod",&storage,Some(storage.saves.join("cnk3d")));
 log_stage(c"R.boot.machine.ok");
 for &name in &fufs::EMBEDDED_ASSET_PATHS{
  let key=CString::new(name).map_err(|e|e.to_string())?;
  let bytes=unsafe{asset(lookup,&key)}?;
  for alias in fufs::brew_aliases(name){machine.mount_embedded(&alias,bytes);}
 }
 log_stage(c"R.boot.mounted");
 machine.set_installed_applets([(clsid,"274214".to_string())]);
 machine.usa_placa(false,None);
 // Full software 3D rasterization is the expensive part of the Vita port.
 // Zeebx supports up to 4x INTERNAL reduction; the final RGB565 frame
 // remains 640x480, preserving the original layout and input coordinates.
 // High quality (2x) is opt-in via VitaShell without rebuilding the VPK.
 let quality=std::path::Path::new("ux0:data/cnk_vita_quality.enable").is_file();
 let reduction=if quality {2} else {4};
 machine.define_reducao(reduction);
 if quality {log_stage(c"R.video.software_3d.quality_2x");}
 else {log_stage(c"R.video.software_3d.performance_4x");}
 log_stage(c"R.boot.run.enter");
 let result=machine.run(BOOT_BUDGET).map_err(|e|e.to_string())?;
 log_stage(c"R.boot.run.returned");
 if !matches!(result,Outcome::Returned{code:0}){return Err(format!("AEEMod_Load: {result:?}"));}
 log_stage(c"R.boot.applet.create.deferred");
 // Diagnostic PPM snapshots each write almost 1MiB through Vita storage,
 // interrupting game execution. Keep exact scene checks and log timestamps,
 // but enable those large writes only after an explicit VitaShell opt-in.
 let capture_enabled=std::path::Path::new("ux0:data/cnk_vita_capture.enable").is_file();
 if capture_enabled {log_stage(c"R.capture.ppm.opt_in.enabled");}
 else {log_stage(c"R.capture.ppm.disabled_for_gameplay");}
 // Opt-in hardware replay of the exact input sequence proven to reach
 // the original game menu/race on Linux. No synthetic graphics or assets.
 let autotest_enabled=std::path::Path::new("ux0:data/cnk_vita_autotest.enable").is_file();
 if autotest_enabled {log_stage(c"R.autotest.controls.enabled");}
 else {log_stage(c"R.autotest.controls.disabled");}
 Ok(Running{machine,applet:None,pad_before:Pad::default(),
  clsid,creation_started:false,creation_finished:false,creation_polls:0,
  frame:Vec::new(),frame565:Vec::new(),w:0,h:0,logged_first_tick:false,ticks:0,logged_video:false,logged_nonblank:false,post_start_trace:0,next_report_ms:1_000,menu_capture_saved:false,race_capture_saved:false,capture_enabled,autotest_enabled,next_scene_validation_ms:20_000,verified_menu:false,verified_race:false,_fufs:fufs})
}
fn check(outcomes:&[Outcome])->Result<(),String>{
 for x in outcomes{if !matches!(x,Outcome::Returned{..}|Outcome::Budget){return Err(format!("Guest stopped: {x:?}"));}}
 Ok(())
}
fn pixel(v:u16)->[u8;4]{
 let r=((v>>11)&31) as u8;let g=((v>>5)&63) as u8;let b=(v&31) as u8;
 [(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255]
}
impl Running{
 // Capture the *actual* RGB pixels from the game, not a synthetic menu.
 // These small, bounded on-device snapshots separate emulation faults from
 // VitaGL/display faults and can be extracted with VitaShell if needed.
 fn save_guest_ppm(&self,path:&str)->std::io::Result<()>{
  use std::io::{BufWriter,Write};
  let expected=(self.w as usize)
   .checked_mul(self.h as usize)
   .ok_or_else(||std::io::Error::other("frame size overflow"))?;
  if self.w==0||self.h==0||self.frame565.len()!=expected {
   return Err(std::io::Error::other("missing or incomplete RGB565 frame"));
  }
  let f=std::fs::File::create(path)?;
  let mut out=BufWriter::with_capacity(64*1024,f);
  write!(out,"P6\n{} {}\n255\n",self.w,self.h)?;
  let mut rgb=[0u8;4096*3];
  for chunk in self.frame565.chunks(4096){
   let mut end=0usize;
   for &v in chunk{
    let px=pixel(v);
    rgb[end..end+3].copy_from_slice(&px[..3]);
    end+=3;
   }
   out.write_all(&rgb[..end])?;
  }
  out.flush()
 }
 // Replicate the host probe's RGB565 scene discriminators on the Vita's
 // final RGBA pixels. They are *heuristics*, not gameplay-completion claims.
 // Run this check only on the two diagnostic snapshots, never per frame.
 fn original_scene_colors(&self)->(usize,usize){
  let w=self.w as usize;let h=self.h as usize;
  if w<600||h<480||self.frame565.len()!=w.saturating_mul(h){return (0,0);}
  let mut orange=0usize;let mut road=0usize;
  for y in 230..415{for x in 150..510{
   let [r,g,b,_]=pixel(self.frame565[y*w+x]);
   if r>=160&&(20..=180).contains(&g)&&b<80{orange+=1;}
  }}
  for y in 295..470{for x in 110..530{
   let [r,g,b,_]=pixel(self.frame565[y*w+x]);
   if (165..=245).contains(&r)&&(100..=185).contains(&g)
    &&(90..=200).contains(&b){road+=1;}
  }}
  (orange,road)
 }
 fn log_scene_validation(&self,kind:&str){
  let (orange,road)=self.original_scene_colors();
  let ms=self.machine.clock_ms();
  if let Ok(msg)=CString::new(format!(
   "R.scene.{kind}.clock={ms}.orange={orange}.road={road}"
  )){log_stage(&msg);}
  if kind=="menu"{
   if orange>10_000&&road<3_000{
    log_stage(c"R.scene.menu.visual_match");
   }else{log_stage(c"R.scene.menu.not_matched");}
  }else if kind=="race"{
   if road>30_000&&orange<3_000{
    log_stage(c"R.scene.race.visual_match");
   }else{log_stage(c"R.scene.race.not_matched");}
  }
 }
 fn capture_guest_milestones(&mut self){
  let clock=self.machine.clock_ms();
  // Confirm the actual original menu/race on hardware. The race does not
  // always start by 40s, so sample until it truly appears rather than
  // reporting a false failure based on a single early screenshot.
  if clock>=self.next_scene_validation_ms {
   self.next_scene_validation_ms=clock.saturating_add(5_000);
   let (orange,road)=self.original_scene_colors();
   if !self.verified_menu && clock>=20_000 && orange>10_000 && road<3_000 {
    self.verified_menu=true;
    log_stage(c"R.scene.menu.verified_live");
   }
   if self.verified_menu && !self.verified_race && clock>=35_000 && road>30_000 && orange<3_000 {
    self.verified_race=true;
    log_stage(c"R.scene.race.verified_live");
   }
  }
  if !self.menu_capture_saved && (25_000..30_000).contains(&clock) {
   self.menu_capture_saved=true;
   self.log_scene_validation("menu");
   if self.capture_enabled {
    match self.save_guest_ppm("ux0:data/cnk_vita_menu_25s.ppm"){
     Ok(())=>log_stage(c"R.capture.menu.candidate.saved"),
     Err(e)=>{
      if let Ok(msg)=CString::new(format!("R.capture.menu.failed: {e}")){log_stage(&msg);}
     }
    }
   }
  }
  if !self.race_capture_saved && (40_000..47_000).contains(&clock){
   self.race_capture_saved=true;
   self.log_scene_validation("race");
   if self.capture_enabled {
    match self.save_guest_ppm("ux0:data/cnk_vita_race_40s.ppm"){
     Ok(())=>log_stage(c"R.capture.race.candidate.saved"),
     Err(e)=>{
      if let Ok(msg)=CString::new(format!("R.capture.race.failed: {e}")){log_stage(&msg);}
     }
    }
   }
  }
 }
 // Report real guest-clock/GL progress to persistent Vita logs. The host
 // reproduced the original menu at ~25s guest time; this proves whether the
 // console reaches the same code path rather than guessing from a black screen.
 fn report_timeline(&mut self) {
  let ms=self.machine.clock_ms();
  if ms<self.next_report_ms{return;}
  let swaps=self.machine.gl_swaps();
  let instructions=self.machine.instructions();
  let has_color=self.frame565.iter().any(|&v|v!=0);
  if let Ok(msg)=CString::new(format!(
   "R.timeline.ms={ms} swaps={swaps} ticks={} instructions={instructions} visible={has_color}",
   self.ticks
  )){log_stage(&msg);}
  self.next_report_ms=if ms<5_000 {5_000}
   else {((ms/5_000).saturating_add(1)).saturating_mul(5_000)};
 }
 // Only copy when pixels actually changed. Static screens can perform
 // hundreds of EGL swaps; avoid redundant RGB565 transfers and vitaGL
 // texture uploads without ever skipping guest CPU/event execution.
 fn publish_frame(&mut self)->bool{
  let screen=self.machine.screen();
  if self.w==screen.width() && self.h==screen.height()
      && self.frame565.as_slice()==screen.pixels(){
   return false;
  }
  self.w=screen.width();self.h=screen.height();
  self.frame565.clear();
  self.frame565.extend_from_slice(screen.pixels());
  true
 }
 fn tick(&mut self,input:ZeebxVitaPad)->Result<bool,String>{
  self.ticks=self.ticks.saturating_add(1);
  self.report_timeline();
  let first_step=!self.logged_first_tick;
  self.logged_first_tick=true;
  if first_step { log_stage(c"R.tick.enter"); }
  // Vita: execute only one bounded slice of CreateInstance per main-loop
  // iteration. The PC/regs and output pointer remain intact between polls.
  if !self.creation_finished {
   if self.creation_polls >= MAX_CREATE_POLLS {
    return Err(format!("CreateInstance exceeded {MAX_CREATE_POLLS} bounded polls"));
   }
   let outcome=if !self.creation_started {
    self.creation_started=true;
    log_stage(c"R.create.begin");
    self.machine.create_applet(self.clsid,CREATE_SLICE)
   }else{
    self.machine.resume_create_applet(CREATE_SLICE)
   }.map_err(|e|e.to_string())?;
   self.creation_polls+=1;
   match outcome {
    AppletResult::Called{code:0,applet} if applet!=0 => {
     log_stage(c"R.create.completed");
     self.applet=Some((applet,self.clsid));
     self.creation_finished=true;
    },
    AppletResult::Stopped(Outcome::Budget) => {
     if self.creation_polls==1 {log_stage(c"R.create.budget.first");}
     // Include a real instruction count on Vita, not only the slice number.
     // This distinguishes slow initialization from a non-advancing CPU.
     // Logging only once per 64 calls keeps filesystem overhead negligible.
     if self.creation_polls%64==0 {
      if let Ok(stage)=CString::new(format!(
       "R.create.progress.polls={}.instructions={}",
       self.creation_polls,self.machine.instructions()
      )){log_stage(&stage);}
     }
     return Ok(false);
    },
    other => return Err(format!("CreateInstance: {other:?}"))
   }
  }
  // Hardware-only optional deterministic test. Ordinary players still
  // receive their actual physical Vita controller input untouched.
  let input=if self.autotest_enabled {
   let ms=self.machine.clock_ms();
   let press=(35_000..35_180).contains(&ms)
      || (60_000..60_180).contains(&ms)
      || (90_000..90_180).contains(&ms);
   let steer=if (70_000..85_000).contains(&ms){100}
      else if (95_000..110_000).contains(&ms){-100}else{0};
   ZeebxVitaPad{buttons:if press{1}else{0},axes:[steer,0,0,0]}
  }else{input};
  let next=Pad{buttons:input.buttons & ((1<<zeebx::input::BUTTONS)-1),axes:input.axes.map(|a|a.clamp(-128,127))};
  self.machine.set_port_pad(0,next);
  for (key,down) in zeebx::input::teclas_do_controle(&self.pad_before,&next){self.machine.set_key(key,down);}
  self.pad_before=next;
  if let Some((a,c))=self.applet.take(){
   log_stage(c"R.tick.applet.start.enter");
   let start=self.machine.start_applet(a,c,FRAME_BUDGET).map_err(|e|e.to_string())?;
   if matches!(start,Outcome::Budget) { log_stage(c"R.tick.applet.start.budget"); }
   check(&[start])?;
   log_stage(c"R.tick.applet.start.ok");
   self.post_start_trace=4;
  }
  let trace=self.post_start_trace>0;
  if trace {
   self.post_start_trace-=1;
   log_stage(c"R.trace.post_start.tick.enter");
   log_stage(c"R.trace.queued_update.enter");
  }
  // EVT_APP_START can itself enqueue a frame: consume it before the next
  // comeca_volta()/fecha_volta(), which would discard a single update.
  if let Some(f)=self.machine.toma_quadro_do_update(){
   if trace {log_stage(c"R.trace.queued_update.found");}
   self.w=f.width();self.h=f.height();
   self.frame565.clear();self.frame565.extend_from_slice(f.pixels());
   if !self.logged_video {log_stage(c"R.video.present.start_update");self.logged_video=true;}
   self.capture_guest_milestones();
   return Ok(true);
  }
  if trace {log_stage(c"R.trace.queued_update.done");}
  let before=self.machine.gl_swaps();
  if trace {log_stage(c"R.trace.swaps.read.ok");}
  for _ in 0..FRAME_PASSES{
   if first_step { log_stage(c"R.tick.advance.enter"); }
   if trace {log_stage(c"R.trace.turn.begin");}
   self.machine.comeca_volta();
   if trace {log_stage(c"R.trace.cpu.advance.enter");}
   let outcomes=self.machine.advance(FRAME_BUDGET).map_err(|e|e.to_string())?;
   if trace {log_stage(c"R.trace.cpu.advance.returned");}
   let signals=self.machine.deliver_signals(FRAME_BUDGET).map_err(|e|e.to_string())?;
   if trace {log_stage(c"R.trace.signals.returned");}
   let cb=self.machine.deliver_callbacks(FRAME_BUDGET).map_err(|e|e.to_string())?;
   if trace {log_stage(c"R.trace.callbacks.returned");}
   // IMPORTANT: Zeebx clears the queued framebuffer on fecha_volta()
   // when there was only one IDisplay_Update in the current turn.
   // Take the latest update BEFORE closing this turn, never afterward.
   let mut latest_update=None;
   if trace {log_stage(c"R.trace.update_drain.enter");}
   while let Some(f)=self.machine.toma_quadro_do_update(){ latest_update=Some(f); }
   if trace {log_stage(c"R.trace.update_drain.done");}
   self.machine.fecha_volta();
   if trace {log_stage(c"R.trace.turn.closed");}
   if first_step { log_stage(c"R.tick.advance.ok"); }
   check(&outcomes)?;check(&signals)?;check(&cb)?;
   if self.machine.pediu_para_fechar(){
    let _=self.machine.encerra_applet();return Err("Applet cerrado".into());
   }
   if let Some(f)=latest_update{
    self.w=f.width();self.h=f.height();
   self.frame565.clear();self.frame565.extend_from_slice(f.pixels());
    if !self.logged_video {log_stage(c"R.video.present.display_update");self.logged_video=true;}
    self.capture_guest_milestones();
    return Ok(true);
   }
   if trace {log_stage(c"R.trace.gl_swap_check.enter");}
   if self.machine.gl_swaps()!=before{
    if trace {log_stage(c"R.trace.gl_swap.found");}
    // The BREW CPU display and GLES rasterizer are separate surfaces.
    // Some games swap a valid 3D image while the device bitmap is still
    // black. Read Zeebx's GLES frame before deciding what to publish.
    let screen_blank=self.machine.screen().pixels().iter().all(|&px| px==0);
    let mut used_gl_frame=false;
    let mut frame_changed=false;
    if screen_blank {
     if let Some(gl_frame)=self.machine.gl_frame() {
      if gl_frame.pixels().iter().any(|&px| px!=0) {
       frame_changed=self.w!=gl_frame.width() || self.h!=gl_frame.height()
        || self.frame565.as_slice()!=gl_frame.pixels();
       if frame_changed {
        self.w=gl_frame.width();self.h=gl_frame.height();
        self.frame565.clear();self.frame565.extend_from_slice(gl_frame.pixels());
       }
       used_gl_frame=true;
       if !self.logged_nonblank {log_stage(c"R.video.gl_readback.nonblank");self.logged_nonblank=true;}
      }
     }
    }
    if !used_gl_frame {
     if trace {log_stage(c"R.trace.cpu_frame.copy.enter");}
     frame_changed=self.publish_frame();
     if trace {log_stage(c"R.trace.cpu_frame.copy.done");}
    }
    let nonblank=self.frame565.iter().any(|&px|px!=0);
    if !self.logged_video {
     if nonblank {log_stage(c"R.video.present.egl_swap.nonblank");}
     else {log_stage(c"R.video.present.egl_swap.blank");}
     self.logged_video=true;
    }
    if nonblank && !self.logged_nonblank {
     log_stage(c"R.video.first.nonblack");
     self.logged_nonblank=true;
    }
    self.capture_guest_milestones();
    if trace {log_stage(c"R.trace.post_swap.return");}
    // The guest still advanced and its virtual clock progressed. When this
    // exact frame is already on screen, save a redundant GPU upload/swap.
    return Ok(frame_changed);
   }
  }
  if trace {log_stage(c"R.trace.post_start.tick.done");}
  // Sparse diagnostics. A framebuffer may change even when no present event
  // is emitted; show it occasionally while preserving a bounded CPU workload.
  if self.ticks==1 || self.ticks==20 || self.ticks==100 {
   if self.machine.screen().pixels().iter().any(|&px|px!=0){
    log_stage(c"R.video.fallback.nonzero");
   }else{
    log_stage(c"R.video.fallback.blank");
   }
   self.publish_frame();
   self.capture_guest_milestones();
   return Ok(true);
  }
  Ok(false)
 }
}
#[unsafe(no_mangle)]
pub extern "C" fn zeebx_vita_boot(lookup:Option<EmbeddedLookup>)->i32{
 let Some(lookup)=lookup else{return fail("Lookup nulo")};
 match boot(lookup){Ok(r)=>{let s=global();s.running=Some(r);s.error=None;0},Err(e)=>fail(e)}
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn zeebx_vita_step(
 pad:*const ZeebxVitaPad,rgba:*mut*const u8,w:*mut u32,h:*mut u32
)->i32{
 if pad.is_null()||rgba.is_null()||w.is_null()||h.is_null(){return fail("Puntero nulo");}
 unsafe{*rgba=ptr::null();*w=0;*h=0;}
 let Some(r)=global().running.as_mut()else{return fail("Sin inicializar")};
 match r.tick(unsafe{*pad}){
  Ok(false)=>0,
  Ok(true)=>{
   // RGBA compatibility for existing host tests. The Vita video path
   // requests RGB565 instead and avoids this 640x480x4 expansion.
   r.frame.resize(r.frame565.len()*4,0);
   for (&v,b) in r.frame565.iter().zip(r.frame.chunks_exact_mut(4)){
    b.copy_from_slice(&pixel(v));
   }
   unsafe{*rgba=r.frame.as_ptr();*w=r.w;*h=r.h;}1
  },
  Err(e)=>fail(e)
 }
}
// Native 16-bit guest framebuffer. Caller must render synchronously before
// the next game step, which can reallocate this buffer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn zeebx_vita_step_rgb565(
 pad:*const ZeebxVitaPad,pixels:*mut*const u16,w:*mut u32,h:*mut u32
)->i32{
 if pad.is_null()||pixels.is_null()||w.is_null()||h.is_null(){return fail("Puntero nulo");}
 unsafe{*pixels=ptr::null();*w=0;*h=0;}
 let Some(r)=global().running.as_mut()else{return fail("Sin inicializar")};
 match r.tick(unsafe{*pad}){
  Ok(false)=>0,
  Ok(true)=>{unsafe{*pixels=r.frame565.as_ptr();*w=r.w;*h=r.h;}1},
  Err(e)=>fail(e)
 }
}
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_last_error()->*const c_char{
 global().error.as_ref().map_or(ptr::null(),|e|e.as_ptr())
}
// A zero-frame result can mean "still interpreting the ARM game", NOT idle.
// Report real guest-instruction progress to the Vita main loop so it never
// sleeps while the interpreter is doing productive work.
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_guest_instructions()->u64{
 global().running.as_ref().map_or(0,|r|r.machine.instructions())
}
// Report the guest clock for deterministic FFI parity tests and diagnostics.
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_guest_clock_ms()->u32{
 global().running.as_ref().map_or(0,|r|r.machine.clock_ms())
}
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_guest_gl_swaps()->u32{
 global().running.as_ref().map_or(0,|r|r.machine.gl_swaps())
}
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_shutdown(){global().running=None;}
