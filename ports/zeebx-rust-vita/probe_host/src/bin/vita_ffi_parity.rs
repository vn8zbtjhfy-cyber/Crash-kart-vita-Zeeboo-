//! Exercise the EXACT Vita Rust bridge on a Linux host, not a parallel
//! reimplementation of Machine::advance. The emulator, files, FFI boot,
//! CreateInstance continuation, inputs and RGBA output all come from lib.rs.
//! No PS Vita/vitaGL hardware is emulated here.
use std::{collections::HashMap, env, error::Error, ffi::{CStr,c_char},fs,path::PathBuf,
    sync::{OnceLock,atomic::{AtomicU32,Ordering}},time::{Duration,Instant}};
#[path="../../../rust/zeebx_vita/src/lib.rs"] mod vita_bridge;
use vita_bridge::{EmbeddedAsset,ZeebxVitaPad};
static ASSETS:OnceLock<HashMap<String,&'static [u8]>>=OnceLock::new();
static LOG_TIMELINE:AtomicU32=AtomicU32::new(0);
const PATHS:[&str;12]=[
 "brew/mif/274214.mif","brew/mod/274214/cnk2.mod",
 "brew/mod/274214/data.vfs","brew/mod/274214/data/gfx/game/effects.png",
 "crash_audio/mp3_0.pcm","crash_audio/mp3_1.pcm","crash_audio/mp3_2.pcm",
 "crash_audio/mp3_3.pcm","crash_audio/mp3_4.pcm","crash_audio/mp3_5.pcm",
 "crash_audio/mp3_6.pcm","zeebo_vshader.shbin",
];
#[unsafe(no_mangle)]
pub extern "C" fn zeebx_vita_debug_log(ptr:*const c_char){
 if ptr.is_null(){return;}
 let message=unsafe{CStr::from_ptr(ptr)}.to_string_lossy();
 if message.starts_with("R.timeline.ms="){
  LOG_TIMELINE.fetch_add(1,Ordering::Relaxed);
  println!("VITA_FFI {message}");
 }else if message.starts_with("R.boot.")||message.starts_with("R.create.")
   ||message.starts_with("R.video.")||message.starts_with("R.scene.")
   ||message.starts_with("R.capture.")||message.starts_with("R.tick.applet.") {
  println!("VITA_FFI {message}");
 }
}
unsafe extern "C" fn lookup(ptr:*const c_char)->EmbeddedAsset {
 if ptr.is_null(){return EmbeddedAsset{bytes:std::ptr::null(),size:0};}
 let path=unsafe{CStr::from_ptr(ptr)}.to_string_lossy();
 let value=ASSETS.get().and_then(|map|map.get(path.as_ref()));
 match value {
  Some(&b)=>EmbeddedAsset{bytes:b.as_ptr(),size:b.len()},
  None=>EmbeddedAsset{bytes:std::ptr::null(),size:0},
 }
}
// The same pixel windows and thresholds as the original host probe
// and Vita's Running::original_scene_colors.
fn classify(w:usize,h:usize,rgba:&[u8])->(usize,usize){
 if w<600||h<480||rgba.len()!=w*h*4{return (0,0);}
 let mut orange=0;let mut road=0;
 for y in 230..415 {for x in 150..510 {
  let i=(y*w+x)*4;
  let (r,g,b)=(rgba[i],rgba[i+1],rgba[i+2]);
  if r>=160&&(20..=180).contains(&g)&&b<80{orange+=1;}
 }}
 for y in 295..470 {for x in 110..530 {
  let i=(y*w+x)*4;
  let (r,g,b)=(rgba[i],rgba[i+1],rgba[i+2]);
  if (165..=245).contains(&r)&&(100..=185).contains(&g)
    &&(90..=200).contains(&b){road+=1;}
 }}
 (orange,road)
}
fn main()->Result<(),Box<dyn Error>> {
 let asset_root=PathBuf::from(env::var("CNK_ASSETS").unwrap_or("../assets".into()));
 let mut map=HashMap::new();
 for &path in &PATHS{
  let bytes=fs::read(asset_root.join(path))?;
  let bytes:&'static [u8]=Box::leak(bytes.into_boxed_slice());
  map.insert(path.to_string(),bytes);
 }
 if ASSETS.set(map).is_err(){return Err("assets initialized twice".into());}
 // Emulated Vita's ux0:data is a relative directory on the host.
 fs::create_dir_all("ux0:data")?;
 let auto_test=env::var("CNK_VITA_AUTO_TEST").is_ok_and(|x|x=="1");
 let auto_test_file="ux0:data/cnk_vita_autotest.enable";
 if auto_test {fs::write(auto_test_file,b"")?;}
 else {let _=fs::remove_file(auto_test_file);}
 println!("CNK_VITA_FFI_AUTOTEST_ENABLED={auto_test}");
 println!("CNK_VITA_FFI_BOOT begin exact Rust Vita bridge; hardware_unverified=true");
 if vita_bridge::zeebx_vita_boot(Some(lookup))!=0{
  let e=unsafe{CStr::from_ptr(vita_bridge::zeebx_vita_last_error())};
  return Err(format!("Vita bridge boot failure: {}",e.to_string_lossy()).into());
 }
 let mut menu=false;let mut race=false;let mut frames=0usize;
 let use_rgb565=env::var("CNK_VITA_PIXEL_FORMAT").is_ok_and(|x|x=="rgb565");
 println!("VITA_FFI_PIXEL_FORMAT={}",if use_rgb565{"rgb565"}else{"rgba"});
 let start=Instant::now();
 let mut last_log_ms=0u32;
 for step in 0..100_000u32 {
  let ms=vita_bridge::zeebx_vita_guest_clock_ms();
  if start.elapsed()>Duration::from_secs(190){
   println!("VITA_FFI_TIMEOUT step={step} ms={ms} menu={menu} race={race}");
   break;
  }
  // Exact button 0 and analog states used in the proven host gameplay.
  let pressed=(35_000..35_180).contains(&ms)
      ||(60_000..60_180).contains(&ms)
      ||(90_000..90_180).contains(&ms);
  let steer=if (70_000..85_000).contains(&ms){100}
      else if (95_000..110_000).contains(&ms){-100} else {0};
  let pad=if auto_test {ZeebxVitaPad{buttons:0,axes:[0;4]}} else {ZeebxVitaPad{buttons:if pressed{1}else{0},axes:[steer,0,0,0]}};
  let mut buf=std::ptr::null();
  let mut w=0u32;let mut h=0u32;
  let mut pixels565=std::ptr::null();
  let result=if use_rgb565 {
   unsafe{vita_bridge::zeebx_vita_step_rgb565(&pad,&mut pixels565,&mut w,&mut h)}
  }else{
   unsafe{vita_bridge::zeebx_vita_step(&pad,&mut buf,&mut w,&mut h)}
  };
  if result<0{
   let ptr=vita_bridge::zeebx_vita_last_error();
   let msg=if ptr.is_null(){"unknown".into()}
       else{unsafe{CStr::from_ptr(ptr)}.to_string_lossy().into_owned()};
   return Err(format!("Vita bridge step={step} guest_ms={ms}: {msg}").into());
  }
  if result==1 {
   if !use_rgb565&&buf.is_null(){return Err("Vita RGBA step reported a null frame".into());}
   if use_rgb565&&pixels565.is_null(){return Err("Vita RGB565 step reported a null frame".into());}
   let n=(w as usize).checked_mul(h as usize).ok_or("frame dimensions overflow")?;
   // Convert exclusively in the Linux test for scene parity. PS Vita
   // uploads the guest's native RGB565 without the CPU-side conversion.
   let converted;
   let rgba:&[u8]=if use_rgb565 {
    let px=unsafe{std::slice::from_raw_parts(pixels565,n)};
    converted=px.iter().flat_map(|&v|{
     let r=((v>>11)&31) as u8;let g=((v>>5)&63) as u8;let b=(v&31) as u8;
     [(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255]
    }).collect::<Vec<u8>>();
    &converted
   }else{
    unsafe{std::slice::from_raw_parts(buf,n*4)}
   };
   frames+=1;
   if ms>=20_000&&!menu {
    let (orange,road)=classify(w as usize,h as usize,rgba);
    if orange>10_000&&road<3_000 {
     menu=true;println!("VITA_FFI_MAIN_MENU_VERIFIED clock={ms} frame={frames} orange={orange}");
    }
   }
   if ms>=39_000&&!race {
    let (orange,road)=classify(w as usize,h as usize,rgba);
    if road>30_000&&orange<3_000{
     race=true;println!("VITA_FFI_RACE_VERIFIED clock={ms} frame={frames} road={road}");
    }
   }
  }
  if ms>=last_log_ms.saturating_add(10_000){
    last_log_ms=ms;
    println!("VITA_FFI_PROGRESS step={step} ms={ms} frames={frames} swaps={} menu={menu} race={race}",
      vita_bridge::zeebx_vita_guest_gl_swaps());
  }
  if menu&&race {break;}
 }
 vita_bridge::zeebx_vita_shutdown();
 if auto_test {let _=fs::remove_file(auto_test_file);}
 println!("VITA_FFI_FINAL menu={menu} race={race} frames={frames} guest_log_events={}",
   LOG_TIMELINE.load(Ordering::Relaxed));
 if !menu {return Err("Actual Vita Rust bridge did not render the original main menu".into());}
 if !race {return Err("Actual Vita Rust bridge did not reach a kart racing scene".into());}
 println!("VITA_FFI_PARITY_PROOF host=true vita_hardware_unverified=true");
 Ok(())
}
