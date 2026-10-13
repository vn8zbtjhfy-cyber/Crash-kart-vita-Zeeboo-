//! Reproduction harness: the original game and ARM interpreter, but on a
//! host with enough RAM and without vitaGL. No claim of playability on Vita.
//! This measures why CreateInstance consumes many emulated instructions.
use std::{env, error::Error, fs, path::{Path, PathBuf}, time::{Duration, Instant}};
use zeebx::{
    cpu::{CpuBackend, Reg, interpretador::Interpretador},
    loader::{self, miffile::MifFile, modfile::ModImage},
    machine::{AppletResult, Machine, Outcome},
    storage::StoragePaths,
};
const MOD:&str = "brew/mod/274214/cnk2.mod";
const MIF:&str = "brew/mif/274214.mif";
const FILES:[&str;12] = [
    "brew/mif/274214.mif",
    MOD,"brew/mod/274214/data.vfs",
    "brew/mod/274214/data/gfx/game/effects.png",
    "crash_audio/mp3_0.pcm","crash_audio/mp3_1.pcm","crash_audio/mp3_2.pcm",
    "crash_audio/mp3_3.pcm","crash_audio/mp3_4.pcm","crash_audio/mp3_5.pcm",
    "crash_audio/mp3_6.pcm","zeebo_vshader.shbin"
];
fn aliases(path: &str) -> Vec<String> {
    let mut names=vec![path.to_string()];
    if let Some(s)=path.strip_prefix("brew/mod/274214/") {names.push(s.to_string());}
    if let Some(s)=path.strip_prefix("brew/mif/") {names.push(s.to_string());}
    if let Some(s)=path.rsplit('/').next(){
        if !names.iter().any(|v|v==s){names.push(s.to_string());}
    }
    names
}
fn read(root:&Path,name:&str)->Result<Vec<u8>,Box<dyn Error>> {
    Ok(fs::read(root.join(name))?)
}
// Save the actual Zeebx RGB565 guest framebuffer for visual verification.
// This is a lossless PPM snapshot, even if the guest never requests a flip.
fn save_guest_screen(machine:&Machine<Interpretador>, filename:&str)->Result<(),Box<dyn Error>>{
    let screen=machine.screen();
    let w=screen.width(); let h=screen.height();
    let mut ppm=format!("P6\\n{w} {h}\\n255\\n").into_bytes();
    for &px in screen.pixels(){
        let r=((px>>11)&31) as u8;
        let g=((px>>5)&63) as u8;
        let b=(px&31) as u8;
        ppm.extend_from_slice(&[(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)]);
    }
    fs::write(filename,&ppm)?;
    println!("FRAME_SNAPSHOT path={filename} dimensions={w}x{h} bytes={}",ppm.len());
    Ok(())
}
fn main()->Result<(),Box<dyn Error>> {
    let root=PathBuf::from(env::var("CNK_ASSETS").unwrap_or("../assets".into()));
    let start=Instant::now();
    println!("CNK host probe: Zeebx interpreter baseline (not Vita), assets={}",root.display());
    let mif=read(&root,MIF)?;
    let clsid=MifFile::parse(&mif)?.main_applet().ok_or("MIF without applet")?;
    let image=ModImage::parse(read(&root,MOD)?)?;
    let loaded=loader::load_with(&image,&[])?;
    let cpu=Interpretador::new()?;
    let storage=StoragePaths::from_root("/tmp/cnk_zeebx_probe");
    let mut machine=Machine::new_with_storage(cpu,loaded,"app0:/virtual-mod",&storage,
       Some(storage.saves.join("cnk3d")));
    for name in FILES {
        let data=read(&root,name)?;
        let mem: &'static [u8]=Box::leak(data.into_boxed_slice());
        for alias in aliases(name){machine.mount_embedded(&alias,mem);}
    }
    machine.set_installed_applets([(clsid,"274214".to_string())]);
    machine.usa_placa(false,None);
    println!("AEEMod_Load.enter clsid={clsid:#x}");
    let outcome=machine.run(20_000_000)?;
    println!("AEEMod_Load.outcome={outcome:?} instr={} pc={:#010x} t={:?}",
        machine.instructions(),machine.cpu().read_reg(Reg::Pc),start.elapsed());
    if !matches!(outcome,Outcome::Returned {code:0}) {
        return Err(format!("AEEMod_Load did not return 0: {outcome:?}").into());
    }
    let initial_budget=250_000;
    let mut result=machine.create_applet(clsid,initial_budget)?;
    println!("CreateInstance.initial={result:?} instr={} pc={:#010x} clock={}ms",
        machine.instructions(),machine.cpu().read_reg(Reg::Pc),machine.clock_ms());
    // Normal Machine::advance performs continuation when Budget saved a PC.
    // Do NOT restart create_applet; that would destroy the caller state.
    // Unlike the earlier 64-slice probe, this covers a much longer
    // initialization window. The explicit timeout prevents a runaway test.
    let mut polls=0u32;
    let mut finished=false;
    for i in 1..=4096u32 {
        if !matches!(result,AppletResult::Stopped(Outcome::Budget)) {finished=true;break;}
        if start.elapsed()>Duration::from_secs(45) {
            println!("TIMEOUT: CreateInstance still executing after 45 seconds");
            break;
        }
        let batch=machine.advance(250_000)?;
        let Some(step)=batch.into_iter().next() else{
            println!("resume[{i}] no deferred execution found");break
        };
        polls=i;
        if i<=8 || i.is_power_of_two() || i%128==0 || !matches!(step,Outcome::Budget){
            println!("resume[{i}] outcome={step:?} instr={} pc={:#010x} clock={}ms elapsed={:?}",
                machine.instructions(),machine.cpu().read_reg(Reg::Pc),machine.clock_ms(),start.elapsed());
        }
        result=AppletResult::Stopped(step);
        if !matches!(result,AppletResult::Stopped(Outcome::Budget)){finished=true;break;}
    }
    println!("FINAL finished={finished} polls={polls} result={result:?} instr={} pc={:#010x} virtual_clock={}ms elapsed={:?}",
        machine.instructions(),machine.cpu().read_reg(Reg::Pc),machine.clock_ms(),start.elapsed());
    if !matches!(result,AppletResult::Stopped(Outcome::Returned{code:0})) {
        return Err(format!("CreateInstance did not return success after {polls} polls: {result:?}").into());
    }
    let applet=machine.host_probe_created_applet()?;
    println!("CreateInstance APPLET_POINTER={applet:#010x}");
    if applet==0 { return Err("CreateInstance returned null applet pointer".into()); }
    let start_outcome=machine.start_applet(applet,clsid,500_000)?;
    println!("EVT_APP_START outcome={start_outcome:?} instr={} clock={}ms",
        machine.instructions(),machine.clock_ms());
    let baseline=machine.gl_swaps();
    let mut first_frame=false;
    let play_start=Instant::now();
    for i in 1..=2000u32 {
        if play_start.elapsed()>Duration::from_secs(30){println!("PLAY START timeout");break;}
        machine.comeca_volta();
        let outcomes=machine.advance(500_000)?;
        let _signals=machine.deliver_signals(500_000)?;
        let _callbacks=machine.deliver_callbacks(500_000)?;
        let mut updates=0;
        while machine.toma_quadro_do_update().is_some(){updates+=1;}
        machine.fecha_volta();
        let swaps=machine.gl_swaps();
        if updates>0 || swaps>baseline{
            println!("FIRST_FRAME iteration={i} updates={updates} swaps={swaps} instr={} clock={}ms elapsed={:?}",
                machine.instructions(),machine.clock_ms(),play_start.elapsed());
            first_frame=true;
            break;
        }
        if i<=4 || i.is_power_of_two() || i%256==0 {
            let nonzero=machine.screen().pixels().iter().any(|&px|px!=0);
            println!("PLAY[{i}] outcome={outcomes:?} instr={} clock={}ms screen_nonzero={} elapsed={:?}",
                machine.instructions(),machine.clock_ms(),nonzero,play_start.elapsed());
        }
    }
    save_guest_screen(&machine,"first_frame.ppm")?;
    println!("FINAL_RENDER frame_seen={first_frame} total_instr={} t={:?}",machine.instructions(),start.elapsed());
    if !first_frame { return Err("No IDisplay_Update or GL swap observed on host".into()); }
    Ok(())
}
