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
#[repr(C)]#[derive(Clone,Copy)]pub struct EmbeddedAsset{bytes:*const u8,size:usize}
type EmbeddedLookup=unsafe extern "C" fn(*const c_char)->EmbeddedAsset;
#[repr(C)]#[derive(Clone,Copy)]pub struct ZeebxVitaPad{buttons:u32,axes:[i32;4]}
const MOD:&CStr=c"brew/mod/274214/cnk2.mod";
const VFS:&CStr=c"brew/mod/274214/data.vfs";
const MIF:&CStr=c"brew/mif/274214.mif";
const BUDGET:u64=20_000_000;
struct Running{
 machine:Machine<Interpretador>,applet:Option<(u32,u32)>,pad_before:Pad,
 frame:Vec<u8>,w:u32,h:u32,logged_first_tick:bool,_fufs:FufsView
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
 log_stage(c"R.boot.run.enter");
 let result=machine.run(BUDGET).map_err(|e|e.to_string())?;
 log_stage(c"R.boot.run.returned");
 if !matches!(result,Outcome::Returned{code:0}){return Err(format!("AEEMod_Load: {result:?}"));}
 log_stage(c"R.boot.applet.create.enter");
 let applet=match machine.create_applet(clsid,BUDGET){
  Ok(AppletResult::Called{code:0,applet}) if applet!=0=>applet,
  Ok(x)=>return Err(format!("CreateInstance: {x:?}")),
  Err(e)=>return Err(e.to_string())
 };
 log_stage(c"R.boot.applet.create.ok");
 Ok(Running{machine,applet:Some((applet,clsid)),pad_before:Pad::default(),frame:Vec::new(),w:0,h:0,logged_first_tick:false,_fufs:fufs})
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
 fn publish_frame(&mut self){
  let screen=self.machine.screen();self.w=screen.width();self.h=screen.height();
  self.frame.resize(screen.pixels().len()*4,0);
  for (&v,b) in screen.pixels().iter().zip(self.frame.chunks_exact_mut(4)){b.copy_from_slice(&pixel(v));}
 }
 fn tick(&mut self,input:ZeebxVitaPad)->Result<bool,String>{
  let first_step=!self.logged_first_tick;
  self.logged_first_tick=true;
  if first_step { log_stage(c"R.tick.enter"); }
  let next=Pad{buttons:input.buttons & ((1<<zeebx::input::BUTTONS)-1),axes:input.axes.map(|a|a.clamp(-128,127))};
  self.machine.set_port_pad(0,next);
  for (key,down) in zeebx::input::teclas_do_controle(&self.pad_before,&next){self.machine.set_key(key,down);}
  self.pad_before=next;
  if let Some((a,c))=self.applet.take(){
   log_stage(c"R.tick.applet.start.enter");
   let start=self.machine.start_applet(a,c,BUDGET).map_err(|e|e.to_string())?;check(&[start])?;
   log_stage(c"R.tick.applet.start.ok");
  }
  let before=self.machine.gl_swaps();
  for _ in 0..16{
   if self.machine.tem_quadros_do_update(){
    if let Some(f)=self.machine.toma_quadro_do_update(){
     self.w=f.width();self.h=f.height();self.frame.resize(f.pixels().len()*4,0);
     for (&v,b) in f.pixels().iter().zip(self.frame.chunks_exact_mut(4)){b.copy_from_slice(&pixel(v));}
     return Ok(true);
    }
   }
   if first_step { log_stage(c"R.tick.advance.enter"); }
   self.machine.comeca_volta();
   let outcomes=self.machine.advance(BUDGET).map_err(|e|e.to_string())?;
   let signals=self.machine.deliver_signals(BUDGET).map_err(|e|e.to_string())?;
   let cb=self.machine.deliver_callbacks(BUDGET).map_err(|e|e.to_string())?;
   self.machine.fecha_volta();
   if first_step { log_stage(c"R.tick.advance.ok"); }
   check(&outcomes)?;check(&signals)?;check(&cb)?;
   if self.machine.pediu_para_fechar(){let _=self.machine.encerra_applet();return Err("Applet cerrado".into());}
   if self.machine.gl_swaps()!=before{self.publish_frame();return Ok(true);}
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
  Ok(true)=>{unsafe{*rgba=r.frame.as_ptr();*w=r.w;*h=r.h;}1},
  Err(e)=>fail(e)
 }
}
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_last_error()->*const c_char{
 global().error.as_ref().map_or(ptr::null(),|e|e.as_ptr())
}
#[unsafe(no_mangle)]pub extern "C" fn zeebx_vita_shutdown(){global().running=None;}
