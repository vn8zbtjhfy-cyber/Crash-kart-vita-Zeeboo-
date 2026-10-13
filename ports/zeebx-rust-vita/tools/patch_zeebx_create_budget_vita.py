#!/usr/bin/env python3
"""Vita-only resumable BREW CreateInstance without blocking the host boot.

The original port retried 24 giant instruction slices in one synchronous call.
This patch exposes a Vita-only method to resume from the saved guest PC/regs,
allowing the bridge to yield to the PS Vita process between small instruction
slices. Desktop behavior is unchanged.
"""
from pathlib import Path
import sys

ORIGINAL = """        let outcome = self.call_guest(
            create_instance,
            [module_ptr, self.module.shell, clsid, out_applet],
            budget,
        )?;
        match outcome {
            Outcome::Returned { code } => Ok(AppletResult::Called {
                code,
                applet: self.cpu.read_u32(out_applet)?,
            }),
            other => Ok(AppletResult::Stopped(other)),
        }
    }
}"""

PATCHED = """        // CNK_VITA_RESUME_CREATE_V2
        // Never call create_applet again on Budget: its output and arguments
        // must survive exactly as saved by Machine::execute.
        let outcome = self.call_guest(
            create_instance,
            [module_ptr, self.module.shell, clsid, out_applet],
            budget,
        )?;
        match outcome {
            Outcome::Returned { code } => Ok(AppletResult::Called {
                code,
                applet: self.cpu.read_u32(out_applet)?,
            }),
            other => Ok(AppletResult::Stopped(other)),
        }
    }

    /// Resume the exact saved call without restarting its constructor.
    #[cfg(any(target_os = "vita", cnk_host_probe))]
    pub fn resume_create_applet(&mut self, budget: u64) -> Result<AppletResult, CpuError> {
        let Some(outcome) = self.retoma_trecho(budget)? else {
            return Err(CpuError("CreateInstance lost its saved guest context".into()));
        };
        let out_applet = self.module.out_module + 4;
        match outcome {
            Outcome::Returned { code } => Ok(AppletResult::Called {
                code,
                applet: self.cpu.read_u32(out_applet)?,
            }),
            other => Ok(AppletResult::Stopped(other)),
        }
    }
}"""

def main(root: Path) -> None:
    p = root / "src/machine/shell.rs"
    text = p.read_text(encoding="utf-8")
    if "CNK_VITA_RESUME_CREATE_V2" in text:
        print("Vita continuation patch already applied")
        return
    matches = text.count(ORIGINAL)
    if matches != 1:
        raise RuntimeError(f"Upstream CreateInstance changed: {matches} anchors")
    p.write_text(text.replace(ORIGINAL, PATCHED, 1), encoding="utf-8")
    print("Vita CreateInstance: resumable across separate host ticks")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_zeebx_create_budget_vita.py zeebx-emu")
    main(Path(sys.argv[1]))
