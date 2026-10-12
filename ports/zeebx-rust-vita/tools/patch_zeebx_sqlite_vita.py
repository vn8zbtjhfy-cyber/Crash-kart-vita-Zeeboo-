#!/usr/bin/env python3
"""Zeebx Vita: keep desktop SQLite, omit native SQLite on the Vita core.

Upstream rusqlite already sets bundled; SQLite's Unix VFS nevertheless needs
sys/ioctl.h, absent from VitaSDK. Cargo --no-default-features now excludes the
optional SQLite dependency and compiles explicit SQL stubs.
The twelve static Crash FUFS assets do not depend on SQLite.

Run AFTER cloning upstream Zeebx development; fail fast if source changes.
"""
from pathlib import Path
import sys

def replace_exact(path: Path, old: str, new: str) -> bool:
    contents = path.read_text(encoding="utf-8")
    if new in contents:
        print("Already patched:", path)
        return False
    if contents.count(old) != 1:
        raise RuntimeError(f"Unexpected upstream syntax in {path}: {contents.count(old)} matches")
    path.write_text(contents.replace(old, new, 1), encoding="utf-8")
    print("Patched:", path)
    return True

def main(root: Path):
    cargo = root / "Cargo.toml"
    brew = root / "src/brew/mod.rs"
    ui = root / "src/ui/acervo.rs"
    ponte = root / "src/ponte.rs"
    for f in (cargo, brew, ui, ponte):
        if not f.is_file():
            raise RuntimeError("Missing Zeebx file: " + str(f))

    # Desktop builds retain upstream DB semantics. On Vita the consumer
    # Cargo.toml uses default-features=false, so sqlite stays disabled.
    replace_exact(cargo,
        'default = ["desktop"]',
        'default = ["desktop", "sqlite"]\nsqlite = ["dep:rusqlite"]')
    replace_exact(cargo,
        'rusqlite = { version = "0.37", features = ["bundled"] }',
        'rusqlite = { version = "0.37", optional = true, features = ["bundled"] }')

    # Keep SQL interface types for Machine::sql_call / save / shell. Opening a
    # database returns an error; no synthetic success or invented rows.
    replace_exact(brew,
        'pub mod sql;',
        '#[cfg(feature = "sqlite")]\npub mod sql;\n'
        '#[cfg(not(feature = "sqlite"))]\n#[path = "sql_vita.rs"]\npub mod sql;')
    stub = root / "src/brew/sql_vita.rs"
    content = '''//! SQL not available in the PS Vita experimental build.
use std::path::{Path, PathBuf};
use crate::library::CatalogIndex;

#[derive(Debug)]
pub struct Database { caminho: PathBuf }
#[derive(Debug)]
pub struct Row {
    pub values: Vec<Option<String>>,
    pub names: Vec<String>,
}
impl Database {
    pub fn open(_path: &Path) -> Result<Self, String> {
        Err("SQLMGR unavailable on experimental PS Vita build".to_owned())
    }
    pub fn caminho(&self) -> &Path { &self.caminho }
    pub fn exec(&self, _sql: &str) -> Result<Vec<Row>, String> {
        Err("SQLMGR unavailable on experimental PS Vita build".to_owned())
    }
}
pub fn sync_z_wheel_library(
    _packaged: &Path,
    _profile: &Path,
    _catalog: &CatalogIndex,
) -> Result<PathBuf, String> {
    Err("SQLMGR unavailable on experimental PS Vita build".to_owned())
}
'''
    if stub.is_file() and stub.read_text(encoding="utf-8") != content:
        raise RuntimeError("Existing SQL Vita stub differs, refusing overwrite")
    stub.write_text(content, encoding="utf-8")
    print("Stub ready:", stub)

    # Z-Wheel catalogue requires SQLite, but Crash does not. Preserve all
    # public types and fall back to absent catalog when database disabled.
    replace_exact(ui,
        '    pub fn carrega(caminho: &Path) -> Option<Self> {',
        '''    #[cfg(feature = "sqlite")]
    pub fn carrega(caminho: &Path) -> Option<Self> {''')
    replace_exact(ui,
        '    pub fn ficha(&self, class_id: u32) -> Option<&Ficha> {',
        '''    #[cfg(not(feature = "sqlite"))]
    pub fn carrega(_caminho: &Path) -> Option<Self> {
        None
    }
    pub fn ficha(&self, class_id: u32) -> Option<&Ficha> {''')
    for name in ("abre_copia", "le_cenas", "le_banco"):
        replace_exact(ui,
            "fn " + name + "(",
            '#[cfg(feature = "sqlite")]\nfn ' + name + "(")

    # Zeeboids-only helper, irrelevant to Crash.
    replace_exact(ponte,
        'pub fn liberar_sincronizacao(aparelho: &std::path::Path) -> Result<String, String> {',
        '''#[cfg(feature = "sqlite")]
pub fn liberar_sincronizacao(aparelho: &std::path::Path) -> Result<String, String> {''')
    replace_exact(ponte,
        '#[cfg(test)]\nmod tests {',
        '''#[cfg(not(feature = "sqlite"))]
pub fn liberar_sincronizacao(_aparelho: &std::path::Path) -> Result<String, String> {
    Err("Zeeboids database helper unavailable in Vita build".into())
}

#[cfg(test)]
mod tests {''')

    print("Vita: SQLite excluded with default-features=false")
    print("Desktop: SQLite enabled by default with bundled build")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: python3 patch_zeebx_sqlite_vita.py zeebx-emu")
    main(Path(sys.argv[1]).resolve())
