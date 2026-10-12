#!/usr/bin/env python3
"""Memory-backed read-only IFile bridge for Zeebx. Fail if upstream anchors change."""
import sys
from pathlib import Path
def repl(s,a,b,label):
    if s.count(a)!=1:raise RuntimeError(f"{label}: expected one anchor, got {s.count(a)}")
    return s.replace(a,b,1)
def main(root):
    root=Path(root)
    m=root/'src/machine/mod.rs'
    f=root/'src/machine/file.rs'
    z=root/'src/machine/zip.rs'
    files=[m,f,z]
    for path in files:
        if not path.is_file():raise RuntimeError(f"Missing {path}")
    mod,src,stream=[x.read_text() for x in files]
    tag='ZEEBX_VITA_MEMORY_IFILE_V1'
    if all(tag in s for s in (mod,src,stream)):return
    if any(tag in s for s in (mod,src,stream)):raise RuntimeError('Partial patch')
    mod=repl(mod,'/// Um callback do guest: a função e o contexto que ela recebe.',
    '''// ZEEBX_VITA_MEMORY_IFILE_V1
#[derive(Debug)]
struct EmbeddedOpenFile { bytes: &'static [u8], pos: usize, guest_path: String }
/// Um callback do guest: a função e o contexto que ela recebe.''','new type')
    mod=repl(mod,'    open_files: HashMap<u32, OpenFile>,',
    '''    open_files: HashMap<u32, OpenFile>,
    embedded_assets: HashMap<String, &'static [u8]>,
    embedded_open_files: HashMap<u32, EmbeddedOpenFile>,''','fields')
    mod=repl(mod,'            open_files: HashMap::new(),',
    '''            open_files: HashMap::new(),
            embedded_assets: HashMap::new(),
            embedded_open_files: HashMap::new(),''','initial state')
    src=repl(src,'impl<C: CpuBackend> Machine<C> {',
    r'''// ZEEBX_VITA_MEMORY_IFILE_V1
impl<C: CpuBackend> Machine<C> {
    pub fn mount_embedded(&mut self, name: &str, bytes: &'static [u8]) {
        let name = name.replace('\\', "/").to_ascii_lowercase();
        if !name.is_empty() && !name.split('/').any(|c| c=="..") {
            self.embedded_assets.insert(name, bytes);
        }
    }
    fn embedded_asset(&self, guest: &str) -> Option<&'static [u8]> {
        let path = guest.replace('\\', "/").to_ascii_lowercase();
        if path.split('/').any(|c| c=="..") {return None;}
        self.embedded_assets.get(&path).copied().or_else(|| {
            let name=path.rsplit('/').next()?;
            self.embedded_assets.get(name).copied()
        })
    }
''','mount bridge')
    src=repl(src,'                    self.open_files.remove(&this);\n                    self.enumerations.remove(&this);',
    '                    self.open_files.remove(&this);\n                    self.embedded_open_files.remove(&this);\n                    self.enumerations.remove(&this);','Release')
    src=repl(src,'            "GetInfo" if iface == Interface::FileMgr => {\n                let guest_path = self.cpu.read_cstring(a1, MAX_STRING);',
    '''            "GetInfo" if iface == Interface::FileMgr => {
                let guest_path = self.cpu.read_cstring(a1, MAX_STRING);
                if let Some(data)=self.embedded_asset(&guest_path){
                    self.write_embedded_info(a2,&guest_path,data.len() as u32)?;
                    return Ok(Some(SUCCESS));
                }''','GetInfoMgr')
    src=repl(src,'                let tamanho = self\n                    .open_files',
    '''                let tamanho=self.embedded_open_files.get(&this)
                    .map(|f|f.bytes.len() as u32).or_else(||self
                    .open_files''','GetInfoEx first')
    src=repl(src,'                    .map(|meta| meta.len() as u32);\n                let Some(tamanho)',
    '                    .map(|meta| meta.len() as u32));\n                let Some(tamanho)','GetInfoEx end')
    src=repl(src,'            "GetInfo" if iface == Interface::File => match self.open_files.get(&this) {',
    '''            "GetInfo" if iface == Interface::File => {
                if let Some(embedded)=self.embedded_open_files.get(&this){
                    let name=embedded.guest_path.clone();let size=embedded.bytes.len() as u32;
                    self.write_embedded_info(a1,&name,size)?;
                    return Ok(Some(SUCCESS));
                }
                match self.open_files.get(&this) {''','GetInfo file')
    src=repl(src,'                None => EBADPARM,\n            },\n            "Test" => {',
    '                None => EBADPARM,\n                }\n            },\n            "Test" => {','GetInfo file close')
    src=repl(src,'            "Test" => {\n                let guest_path = self.cpu.read_cstring(a1, MAX_STRING);',
    '''            "Test" => {
                let guest_path = self.cpu.read_cstring(a1, MAX_STRING);
                if self.embedded_asset(&guest_path).is_some(){return Ok(Some(SUCCESS));}''','Test')
    src=repl(src,'                let read = match self.open_files.get_mut(&this) {\n                    Some(open) => {',
    '''                let read=if let Some(f)=self.embedded_open_files.get_mut(&this){
                    let pos=f.pos.min(f.bytes.len());
                    let n=count.min(f.bytes.len()-pos);
                    buffer[..n].copy_from_slice(&f.bytes[pos..pos+n]);
                    f.pos=pos+n;n
                }else{match self.open_files.get_mut(&this){
                    Some(open) => {''','Read')
    src=repl(src,'                    None => 0,\n                };\n                if read > 0 {',
    '                    None => 0,\n                }};\n                if read > 0 {','Read close')
    src=repl(src,'            "Write" => {\n                let bytes = self.read_bytes(a1, a2)?;',
    '''            "Write" => {
                if self.embedded_open_files.contains_key(&this){return Ok(Some(0));}
                let bytes = self.read_bytes(a1, a2)?;''','Write')
    src=repl(src,'                match self.open_files.get_mut(&this) {\n                    Some(open) => match std::io::Seek::seek(&mut open.file, from) {',
    '''                if let Some(f)=self.embedded_open_files.get_mut(&this){
                    let base=match a1 { SEEK_END=>f.bytes.len() as i64,
                        SEEK_CURRENT=>f.pos as i64,_=>0 };
                    let n=if a1==SEEK_END||a1==SEEK_CURRENT{
                        base.checked_add(position)}else{Some(position.max(0))};
                    match n{
                        Some(x) if x>=0&&x<=u32::MAX as i64=>{
                          f.pos=x as usize;
                          if a1==SEEK_CURRENT&&a2==0{x as u32}else{SUCCESS}
                        },
                        _=>EFAILED,
                    }
                }else{match self.open_files.get_mut(&this){
                    Some(open) => match std::io::Seek::seek(&mut open.file, from) {''','Seek')
    src=repl(src,'                    None => EFAILED,\n                }\n            }\n            "Truncate" => match self.open_files.get_mut(&this) {',
    '                    None => EFAILED,\n                }}\n            }\n            "Truncate" => match self.open_files.get_mut(&this) {','Seek close')
    src=repl(src,'        // A partir daqui vale a resolução do VFS, que decide entre conteúdo e overlay e diz se\n',
    '''        if mode & (OFM_CREATE | OFM_READWRITE | OFM_APPEND)==0{
            if let Some(bytes)=self.embedded_asset(guest_path){
                let handle=self.new_object(Interface::File)?;
                if handle==0{return Ok(0);}
                self.embedded_open_files.insert(handle,EmbeddedOpenFile{
                    bytes,pos:0,guest_path:guest_path.to_string(),
                });
                self.file_error=SUCCESS;return Ok(handle);
            }
        }
        // A partir daqui vale a resolución del VFS, que decide entre conteúdo e overlay e diz se
''','OpenFile')
    src=repl(src,'    pub(super) fn write_file_info(',
    '''    fn write_embedded_info(&mut self,addr:u32,name:&str,len:u32)->Result<(),CpuError>{
        if addr==0{return Ok(());}
        self.cpu.write_u32(addr,FA_NORMAL)?;
        self.cpu.write_u32(addr+4,0)?;
        self.cpu.write_u32(addr+8,len)?;
        let mut b=name.as_bytes().to_vec();b.truncate(MAX_FILE_NAME-1);
        b.resize(MAX_FILE_NAME,0);
        self.cpu.write_mem(addr+12,&b)
    }
    pub(super) fn write_file_info(''','metadata')
    stream=repl(stream,'        // Um \`IFile\` aberto também é um \`IAStream\`, e é assim que o Double Dragon entrega o\n',
    '''        // ZEEBX_VITA_MEMORY_IFILE_V1
        if let Some(f)=self.embedded_open_files.get_mut(&source){
            let pos=f.pos.min(f.bytes.len());
            let bytes=f.bytes[pos..].to_vec();f.pos=f.bytes.len();
            return Some(Ok(bytes));
        }
        // Um \`IFile\` aberto também é um \`IAStream\`, e é assim que o Double Dragon entrega o
'''.replace('\\\`','\`'),'read stream')
    for path,updated in zip(files,(mod,src,stream)):
        path.write_text(updated);print("Patched",path)
if __name__=="__main__":main(sys.argv[1])
