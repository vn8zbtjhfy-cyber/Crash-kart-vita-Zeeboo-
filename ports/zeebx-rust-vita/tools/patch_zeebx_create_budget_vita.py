#!/usr/bin/env python3
"""Resume Vita's BREW CreateInstance when Zeebx returns Outcome::Budget.

Machine::execute() snapshots registers/PC on the normal outermost Budget path.
Machine::retoma_trecho() restores and resumes that SAME call; unlike calling
create_applet again, this does not re-create the applet from scratch.

Limit resumption attempts to avoid an unlimited busy loop if guest never yields.
Desktop behavior remains untouched. Requires boot_trace patch for diagnostics.
"""
from pathlib import Path
import sys

ORIGINAL = """        let outcome = self.call_guest(
            create_instance,
            [module_ptr, self.module.shell, clsid, out_applet],
            budget,
        )?;
        match outcome {"""

PATCHED = """        // CNK_VITA_RESUME_CREATE_V1
        // The normal 20M-instruction slice may stop legitimate game startup.
        // Restore guest PC/regs after Budget rather than calling CreateInstance
        // twice (which would overwrite its output pointer and corrupt state).
        #[cfg(target_os = "vita")]
        let outcome = {
            const MAX_RESUMES: usize = 24;
            let mut stop = self.call_guest(
                create_instance,
                [module_ptr, self.module.shell, clsid, out_applet],
                budget,
            )?;
            for _ in 0..MAX_RESUMES {
                if !matches!(stop, Outcome::Budget) {
                    break;
                }
                super::cnk_vita_trace(c"Z.create.budget.resume");
                match self.retoma_trecho(budget)? {
                    Some(next) => stop = next,
                    None => {
                        super::cnk_vita_trace(c"Z.create.resume.missing");
                        break;
                    }
                }
            }
            if matches!(stop, Outcome::Budget) {
                super::cnk_vita_trace(c"Z.create.budget.exhausted");
            }
            stop
        };
        #[cfg(not(target_os = "vita"))]
        let outcome = self.call_guest(
            create_instance,
            [module_ptr, self.module.shell, clsid, out_applet],
            budget,
        )?;
        match outcome {"""

def main(root: Path):
    p = root / "src/machine/shell.rs"
    txt = p.read_text(encoding="utf-8")
    if "CNK_VITA_RESUME_CREATE_V1" in txt:
        print("Vita CreateInstance resume patch already applied")
        return
    count = txt.count(ORIGINAL)
    if count != 1:
        raise RuntimeError(f"Zeebx create_applet anchor changed: {count} matches")
    # The trace helper in the machine module is compiled before this callback
    # and is needed for safe persisted diagnostic breadcrumbs.
    m = (root/"src/machine/mod.rs").read_text(encoding="utf-8")
    if "CNK_VITA_BOOT_TRACE_V1" not in m:
        raise RuntimeError("Apply boot trace patch before CreateInstance patch")
    p.write_text(txt.replace(ORIGINAL, PATCHED, 1), encoding="utf-8")
    print("Patched Vita CreateInstance: bounded, state-preserving resume after Budget")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_zeebx_create_budget_vita.py zeebx-emu")
    main(Path(sys.argv[1]))
