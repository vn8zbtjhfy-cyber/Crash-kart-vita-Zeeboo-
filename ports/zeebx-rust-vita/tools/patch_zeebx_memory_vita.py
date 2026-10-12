#!/usr/bin/env python3
"""PS Vita: move the already allocated BREW guest memory into ARM interpreter.

Zeebx's loader allocates a 64 MiB guest heap. Interpreter.reset clones each
region (including the 64 MiB heap) when Machine.run starts; on retail PS Vita
that second 64 MiB allocation can abort the process.
Vita Machine.run is a one-shot boot and does not consult module.mem afterwards:
move the regions to the interpreter without copying, leaving the source empty.
Desktop / other platforms retain their original semantics and reset() API.

This patch is intentionally limited to target_os="vita".
"""
from pathlib import Path
import sys

def replace_once(file:Path, old:str, new:str, desc:str)->None:
    src=file.read_text(encoding="utf-8")
    if new in src:
        print("Already patched:",desc)
        return
    n=src.count(old)
    if n != 1:
        raise RuntimeError(f"{desc}: expected 1 anchor, found {n} in {file}")
    file.write_text(src.replace(old,new,1),encoding="utf-8")
    print("Patched:",desc)

def main(root:Path)->None:
    cpu=root/"src/cpu/mod.rs"
    interp=root/"src/cpu/interpretador.rs"
    machine=root/"src/machine/mod.rs"
    for path in [cpu,interp,machine]:
        if not path.is_file():raise RuntimeError("Missing Zeebx source "+str(path))

    replace_once(cpu,
        """    fn reset(&mut self, mem: &GuestMemory) -> Result<(), CpuError>;

    fn read_reg(&self, reg: Reg) -> u32;""",
        """    fn reset(&mut self, mem: &GuestMemory) -> Result<(), CpuError>;

    // CNK_VITA_MOVE_MEMORY_V1: default fallback preserves other backends.
    #[cfg(target_os = "vita")]
    fn reset_owned(&mut self, mem: GuestMemory) -> Result<(), CpuError> {
        self.reset(&mem)
    }

    fn read_reg(&self, reg: Reg) -> u32;""",
        "CPU backend owning reset API")

    replace_once(interp,
        """        self.exclusivo = None;
        Ok(())
    }

    fn read_reg(&self, reg: Reg) -> u32 {""",
        """        self.exclusivo = None;
        Ok(())
    }

    // Vita-only: do not duplicate the 64 MiB guest heap on startup.
    #[cfg(target_os = "vita")]
    fn reset_owned(&mut self, mem: GuestMemory) -> Result<(), CpuError> {
        self.mem = mem;
        self.r = [0; 16];
        self.cpsr = MODO_USUARIO;
        self.instrucoes = 0;
        self.vigias.clear();
        self.semihosting.clear();
        self.exclusivo = None;
        Ok(())
    }

    fn read_reg(&self, reg: Reg) -> u32 {""",
        "ARM interpreter consumes GuestMemory")

    replace_once(machine,
        """        self.cpu.reset(&self.module.mem)?;""",
        """        // CNK_VITA_MOVE_MEMORY_V1
        // The guest memory is used once during boot, then owned by the CPU.
        // Move instead of cloning all regions, in particular the 64 MiB heap.
        #[cfg(target_os = "vita")]
        {
            let mem = std::mem::take(&mut self.module.mem);
            self.cpu.reset_owned(mem)?;
        }
        #[cfg(not(target_os = "vita"))]
        self.cpu.reset(&self.module.mem)?;""",
        "Machine.run one-shot memory handoff")
    print("Vita guest memory handoff now avoids second 64 MiB heap allocation")

if __name__=="__main__":
    if len(sys.argv)!=2:
        raise SystemExit("usage: python3 patch_zeebx_memory_vita.py zeebx-emu")
    main(Path(sys.argv[1]))
