# Avance 117: decodificador Zeebx integrado en el audio nativo

El proyecto ZeebxTeam/zeebx-emu publica su módulo de sonido en
[src/audio/wav.rs](https://github.com/ZeebxTeam/zeebx-emu/blob/f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9/src/audio/wav.rs).
Se adaptó la porción **IMA ADPCM** (tabla de cuantización, estado predictor,
orden de nibbles e intercalado de canales) de Rust a C99, bajo la
**licencia GPL-2.0-or-later de Zeebx**. Esto es reutilización concreta de
código de Zeebx, no un emulador.

## Integración ejecutable

- `src/third_party/zeebx_ima.[ch]` implementa lectura acotada de RIFF IMA
  ADPCM (`wFormatTag=17`), mono y estéreo, con bloques completos, soporte
  del chunk `fact` y PCM16 intercalado para el mixer de PS Vita.
- `cnk_wav_asset_open_memory` prepara audio cargado desde un recurso:
  copia WAV PCM existente o convierte WAV IMA ADPCM a PCM16.
- El cargador real `cnk_wav_asset_load` usado por `cnk_sound_cues_read`
  pasa ahora por esa ruta. La mezcla `cnk_audio_play` y
  `cnk_audio_render` permanece sin cambios.
- Salida válida o fallo sin recursos parciales; hasta 32 MiB de PCM
  decodificado. La fuente original permanece separada del audio producido.
- Prueba `zeebx_ima` con muestras **sintéticas**: mono, estéreo, intercalado,
  fragmentos, recorte con `fact`, fallos de entrada, sonido efectivamente
  reproducido por el mixer y compatibilidad hacia atrás de PCM.
- `third_party/zeebx/LICENSE` y `NOTICE.md` conservan licencia, atribución,
  referencia exacta del código de origen y requisitos para una distribución
  futura de software vinculado al componente GPL.

## Evidencia y limitaciones

- 18 de 18 tests **sin ROM** pasaron en GitHub Actions antes de integrar.
- **No afirmamos que Crash Nitro Kart Zeebo use IMA ADPCM**: Zeebx lo
  documentó para otros juegos. Esta mejora permite cargar más formatos y
  reutilizar directamente un componente del emulador.
- Los sonidos originales, su identidad y su relación con eventos de carrera
  siguen sujetos a comprobación con datos de la versión Zeebo del usuario.
- No hay prueba en consola física. El enlace VitaSDK debe validarse tras
  integrar este cambio, antes de considerarlo compatible con el compilador ARM.
- No se han añadido ROM, `cnk2.mod`, `data.vfs`, modelos, imágenes, música,
  SELF ni VPK.

## El próximo cuello de botella

El objetivo principal sigue siendo identificar de manera verificable
los recursos que aparecen en cada circuito, los personajes y sus sonidos,
para probar una carrera completa en la PS Vita física. Este trabajo de audio
no sustituye esas tareas ni justifica inflar el porcentaje global.
