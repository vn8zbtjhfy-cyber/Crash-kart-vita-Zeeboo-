//! Tabla FUFS y alias de los doce recursos originales del RomFS.
//! Todos los recursos comerciales se montan como memoria inmutable del ELF.
#[derive(Clone,Copy,Debug,PartialEq,Eq)]
pub struct Entry { pub offset:u32, pub id:u32, pub size:u32 }
pub struct FufsView { bytes:&'static [u8], pub entries:Vec<Entry> }
pub const EMBEDDED_ASSET_PATHS:[&str;12]=[
"brew/mif/274214.mif","brew/mod/274214/cnk2.mod",
"brew/mod/274214/data.vfs","brew/mod/274214/data/gfx/game/effects.png",
"crash_audio/mp3_0.pcm","crash_audio/mp3_1.pcm","crash_audio/mp3_2.pcm",
"crash_audio/mp3_3.pcm","crash_audio/mp3_4.pcm","crash_audio/mp3_5.pcm",
"crash_audio/mp3_6.pcm","zeebo_vshader.shbin",
];
fn word(b:&[u8],p:usize)->Result<u32,String>{
let a=b.get(p..p.checked_add(4).ok_or("FUFS overflow")?).ok_or("FUFS truncado")?;
Ok(u32::from_le_bytes(a.try_into().map_err(|_|"FUFS u32")?))
}
impl FufsView{
pub fn parse(bytes:&'static [u8])->Result<Self,String>{
if bytes.len()<12 || !bytes.starts_with(b"FUFS"){return Err("Firma FUFS invalida".into());}
let count=word(bytes,8)? as usize;
if count==0||count>100_000{return Err("Cantidad FUFS invalida".into());}
let table_end=12usize.checked_add(count.checked_mul(12).ok_or("FUFS overflow")?).ok_or("FUFS overflow")?;
if table_end>bytes.len(){return Err("Tabla FUFS truncada".into());}
let mut entries=Vec::with_capacity(count);
for i in 0..count{
let p=12+i*12;let e=Entry{offset:word(bytes,p)?,id:word(bytes,p+4)?,size:word(bytes,p+8)?};
let start=e.offset as usize;let end=start.checked_add(e.size as usize).ok_or("FUFS offset overflow")?;
if start<table_end||end>bytes.len(){return Err(format!("FUFS descriptor {i} fuera de limites"));}
if let Some(prev)=entries.last(){let prev:&Entry=prev;if e.id<=prev.id{return Err("Indice FUFS no ordenado".into());}}
entries.push(e);
}
Ok(Self{bytes,entries})
}
pub fn get_by_id(&self,id:u32)->Option<&'static [u8]>{
let pos=self.entries.binary_search_by_key(&id,|e|e.id).ok()?;
let e=self.entries[pos];let p=e.offset as usize;
self.bytes.get(p..p.checked_add(e.size as usize)?)
}
pub fn read_at(&self,id:u32,position:usize,count:usize)->Option<&'static [u8]>{
let file=self.get_by_id(id)?;
let start=position.min(file.len());
Some(&file[start..start.saturating_add(count).min(file.len())])
}
}
pub fn normalize_brew_path(input:&str)->Option<String>{
let lowercase=input.trim().replace('\\',"/").to_ascii_lowercase();
let p=if let Some(r)=lowercase.strip_prefix("fs:/~/"){r}
else if let Some(r)=lowercase.strip_prefix("fs:/~"){match r.find('/'){Some(i) if r[..i].starts_with("0x")||r[..i].chars().all(|c|c.is_ascii_digit())=>&r[i+1..],_=>r}}
else if let Some(r)=lowercase.strip_prefix("~/"){r}
else if let Some(r)=lowercase.strip_prefix("fs:/"){r}else{&lowercase};
let p=p.trim_start_matches('/');
if p.is_empty()||p.split('/').any(|c|c==".."||c.is_empty()){None}else{Some(p.to_string())}
}
pub fn brew_aliases(canonical:&str)->Vec<String>{
let mut aliases=Vec::new();let Some(p)=normalize_brew_path(canonical) else{return aliases};
aliases.push(p.clone());
if let Some(s)=p.strip_prefix("brew/mod/274214/"){aliases.push(s.to_string());}
if let Some(s)=p.strip_prefix("brew/mif/"){aliases.push(s.to_string());}
if let Some(s)=p.rsplit('/').next(){if !aliases.iter().any(|a|a==s){aliases.push(s.to_string());}}
aliases
}
#[cfg(test)] mod tests {
use super::*;
#[test]fn eof(){let mut b=b"FUFS\x00\x00\x00\x00\x01\x00\x00\x00".to_vec();
b.extend_from_slice(&24u32.to_le_bytes());b.extend_from_slice(&7u32.to_le_bytes());
b.extend_from_slice(&1u32.to_le_bytes());b.push(42);
let f=FufsView::parse(Box::leak(b.into_boxed_slice())).unwrap();
assert_eq!(f.get_by_id(8),None);assert_eq!(f.read_at(7,3,1),Some(&b""[..]));}
}