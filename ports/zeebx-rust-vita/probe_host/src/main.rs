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
// Save actual RGB565 output, not a fake menu. Compare the BREW 2D bitmap
// against the Zeebx OpenGL rasterizer to identify where color is lost.
fn save_rgb565(w:u32,h:u32,pixels:&[u16],filename:&str)->Result<(),Box<dyn Error>>{
    let mut ppm=format!("P6\n{w} {h}\n255\n").into_bytes();
    for &px in pixels{
        let r=((px>>11)&31) as u8;
        let g=((px>>5)&63) as u8;
        let b=(px&31) as u8;
        ppm.extend_from_slice(&[(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)]);
    }
    fs::write(filename,&ppm)?;
    println!("FRAME_SNAPSHOT path={filename} dimensions={w}x{h} bytes={}",ppm.len());
    Ok(())
}
fn save_guest_screen(machine:&Machine<Interpretador>,filename:&str)->Result<(),Box<dyn Error>>{
    let s=machine.screen();
    save_rgb565(s.width(),s.height(),s.pixels(),filename)
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
    let mut last_swaps=baseline;
    let mut saved=0u32;
    let mut first_swap=false;
    let mut last_signature=0u64;
    let mut total_updates=0usize;
    // Intro animation filled all 18 old screenshots in its first seconds.
    // Record milestones through 90+ seconds of emulated gameplay instead.
    let mut next_milestone_ms=1_000u64;
    let mut milestones=0u32;
    let play_start=Instant::now();
    println!("PLAY_EXTENDED begin clock={}ms swaps={baseline}",machine.clock_ms());
    // Run through the Zeebo advertisement / intro, not merely the first swap.
    // Save only visually distinct frames; every image comes directly from the
    // emulated guest, not an illustration or a forced host-side splash.
    for i in 1..=100_000u32 {
        if play_start.elapsed()>Duration::from_secs(125){
            println!("PLAY_EXTENDED timed out after 125s");break;
        }
        machine.comeca_volta();
        let outcomes=machine.advance(500_000)?;
        let signals=machine.deliver_signals(500_000)?;
        let callbacks=machine.deliver_callbacks(500_000)?;
        let mut updates=0usize;
        while machine.toma_quadro_do_update().is_some(){updates+=1;}
        total_updates+=updates;
        machine.fecha_volta();
        let swaps=machine.gl_swaps();
        let presented=updates>0||swaps!=last_swaps;
        last_swaps=swaps;
        if presented {
            first_swap=true;
            // First 4 EGL swaps: save both paths separately. A near-black
            // CPU bitmap does not prove the 3D output is black.
            if swaps>0 && swaps<=4 {
                let (cpu_min,cpu_max)=machine.screen().pixels().iter().fold(
                    (u16::MAX,u16::MIN), |(mn,mx),&v|(mn.min(v),mx.max(v)));
                if let Some(gl)=machine.gl_frame(){
                    let (gl_min,gl_max)=gl.pixels().iter().fold(
                        (u16::MAX,u16::MIN), |(mn,mx),&v|(mn.min(v),mx.max(v)));
                    let out=format!("gles_swap_{swaps:03}.ppm");
                    save_rgb565(gl.width(),gl.height(),gl.pixels(),&out)?;
                    println!("FRAMEBUFFER_DIFFERENCE swap={swaps} cpu_min={cpu_min:#06x} cpu_max={cpu_max:#06x} gl_min={gl_min:#06x} gl_max={gl_max:#06x}");
                } else {
                    println!("FRAMEBUFFER_DIFFERENCE swap={swaps} gl_frame=NONE cpu_min={cpu_min:#06x} cpu_max={cpu_max:#06x}");
                }
            }
            // Save an unmodified guest framebuffer at regular virtual-time
            // milestones. Enables an objective search for the game's menu.
            let guest_ms=machine.clock_ms();
            if guest_ms>=next_milestone_ms && milestones<35 {
                let filename=format!("milestone_{milestones:02}_{guest_ms:06}ms.ppm");
                save_guest_screen(&machine,&filename)?;
                println!("MILESTONE_CAPTURE time={guest_ms}ms swap={swaps} file={filename}");
                milestones+=1;
                next_milestone_ms=if guest_ms<5_000 {guest_ms+2_000}
                    else {guest_ms+5_000};
            }
            let frame=machine.screen();
            let mut checksum=0xcbf29ce484222325u64;
            let mut min=u16::MAX; let mut max=u16::MIN;
            let mut distinct=std::collections::BTreeSet::new();
            for &px in frame.pixels(){
                checksum=(checksum^u64::from(px)).wrapping_mul(0x100000001b3);
                min=min.min(px); max=max.max(px);
                if distinct.len()<64{distinct.insert(px);}
            }
            if checksum!=last_signature {
                if saved<18 {
                    let filename=format!("sequence_{saved:02}_swap_{swaps:04}.ppm");
                    save_guest_screen(&machine,&filename)?;
                    println!("VISUAL_CHANGE i={i} swap={swaps} checksum={checksum:016x} min={min:#06x} max={max:#06x} distinct_colors_at_least={} clock={}ms",
                        distinct.len(),machine.clock_ms());
                    saved+=1;
                }
                last_signature=checksum;
            }
        }
        if i<=5||i.is_power_of_two()||i%512==0{
            println!("PROGRESS i={i} clock={}ms swaps={} instr={} changed_frames={} last_outcomes={outcomes:?} signals={signals:?} callbacks={callbacks:?} t={:?}",
                machine.clock_ms(),swaps,machine.instructions(),saved,play_start.elapsed());
        }
    }
    save_guest_screen(&machine,"last_frame.ppm")?;
    println!("FILES_MISSING {:?}",machine.missing_files());
    println!("GL_IGNORED {:?}",machine.ignored_gl());
    println!("FINAL_RENDER_EXTENDED swaps={} distinct_frames={} milestones={milestones} guest_ms={} updates={total_updates} elapsed={:?}",
        machine.gl_swaps(),saved,machine.clock_ms(),start.elapsed());
    if !first_swap { return Err("Guest never called eglSwapBuffers".into()); }
    Ok(())
}
