#!/usr/bin/env python3
"""Diagnostic trace of Zeebx BREW startup, PS Vita only."""
from pathlib import Path
import sys

def exactly(src, needle, replacement, label):
    n = src.count(needle)
    if n != 1:
        raise RuntimeError(f"Zeebx upstream changed at {label}, found {n} anchors")
    return src.replace(needle, replacement, 1)

def main(root: Path):
    path = root / "src/machine/mod.rs"
    src = path.read_text(encoding="utf-8")
    if "CNK_VITA_BOOT_TRACE_V1" in src:
        print("Vita trace already present")
        return
    trace = """// CNK_VITA_BOOT_TRACE_V1
#[cfg(target_os = "vita")]
fn cnk_vita_trace(stage: &'static std::ffi::CStr) {
    unsafe extern "C" {
        fn zeebx_vita_debug_log(line: *const std::ffi::c_char);
    }
    unsafe { zeebx_vita_debug_log(stage.as_ptr()) }
}
#[cfg(not(target_os = "vita"))]
fn cnk_vita_trace(_: &'static std::ffi::CStr) {}
"""
    src = exactly(src, 'impl<C: CpuBackend> Machine<C> {',
                  trace + '\nimpl<C: CpuBackend> Machine<C> {', 'trace helper')
    src = exactly(src,
        """    pub fn run(&mut self, budget: u64) -> Result<Outcome, CpuError> {
        self.cpu.reset(&self.module.mem)?;
        self.cpu.write_reg(Reg::R0, self.module.shell);""",
        """    pub fn run(&mut self, budget: u64) -> Result<Outcome, CpuError> {
        cnk_vita_trace(c"Z.run.start");
        cnk_vita_trace(c"Z.cpu.reset.enter");
        self.cpu.reset(&self.module.mem)?;
        cnk_vita_trace(c"Z.cpu.reset.ok");
        self.cpu.write_reg(Reg::R0, self.module.shell);""", 'CPU reset')
    src = exactly(src,
        """        self.cpu.write_reg(Reg::Lr, RETURN_MAGIC);

        self.execute(self.module.entry, budget)
    }""",
        """        self.cpu.write_reg(Reg::Lr, RETURN_MAGIC);

        cnk_vita_trace(c"Z.execute.enter");
        let outcome = self.execute(self.module.entry, budget);
        cnk_vita_trace(c"Z.execute.returned");
        outcome
    }""", 'execute entry')
    src = exactly(src,
        """        self.orcamento = budget;
        loop {
            let gasto = self.cpu.instructions().saturating_sub(comeco);""",
        """        self.orcamento = budget;
        let mut first_cpu_run = true;
        let mut first_api_call = true;
        loop {
            let gasto = self.cpu.instructions().saturating_sub(comeco);""",
        'first-use flags')
    src = exactly(src,
        """            match self.cpu.run(pc, fatia)? {
                StopReason::ApiCall { addr }""",
        """            if first_cpu_run { cnk_vita_trace(c"Z.cpu.run.enter"); }
            let stop = self.cpu.run(pc, fatia)?;
            if first_cpu_run {
                cnk_vita_trace(c"Z.cpu.run.returned");
                first_cpu_run = false;
            }
            match stop {
                StopReason::ApiCall { addr }""",
        'ARM interpreter call')
    src = exactly(src,
        """                StopReason::ApiCall { addr } => match self.dispatch(addr)? {
                    Some(result) => {""",
        """                StopReason::ApiCall { addr } => match {
                    let first = first_api_call;
                    if first { cnk_vita_trace(c"Z.api.dispatch.enter"); }
                    let result = self.dispatch(addr)?;
                    if first {
                        cnk_vita_trace(c"Z.api.dispatch.returned");
                        first_api_call = false;
                    }
                    result
                } {
                    Some(result) => {""",
        'BREW API dispatch')
    path.write_text(src, encoding="utf-8")
    print("Vita startup trace patched: reset, interpreter and first API dispatch")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_zeebx_boot_trace_vita.py zeebx-emu")
    main(Path(sys.argv[1]))
