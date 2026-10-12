//! Reproduction harness: the original game and ARM interpreter, but on a
//! host with enough RAM and without vitaGL. No claim of playability on Vita.
//! This measures why CreateInstance consumes many emulated instructions.
use std::{env, error::Error, fs, path::{Path, PathBuf}, time::Instant};
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
    for i in 1..=64u32 {
        if !matches!(result,AppletResult::Stopped(Outcome::Budget)){break;}
        let batch=machine.advance(250_000)?;
        let Some(step)=batch.into_iter().next() else{
            println!("resume[{i}] no deferred execution found");break
        };
        if i<=8||i%8==0||!matches!(step,Outcome::Budget){
            println!("resume[{i}] outcome={step:?} instr={} pc={:#010x} clock={}ms elapsed={:?}",
                machine.instructions(),machine.cpu().read_reg(Reg::Pc),machine.clock_ms(),start.elapsed());
        }
        result=AppletResult::Stopped(step);
    }
    println!("FINAL last_step={result:?} instr={} t={:?}",machine.instructions(),start.elapsed());
    Ok(())
}
