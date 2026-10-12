# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** del port nativo de Zeebo a PlayStation Vita. **Todavía no es un videojuego completo ni se ha verificado su ejecución en consola.**

## Avance 121: emulador Zeebx completo disponible y ejecución ARM real

**El port nativo de PS Vita sigue intacto.** Se añadió como parte del
mismo repositorio el código fuente completo de
[ZeebxTeam/zeebx-emu](https://github.com/ZeebxTeam/zeebx-emu)
(submódulo Git `third_party/zeebx_emulator`, revisión fija
`f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9`).
Para traer los archivos del submódulo al clonar:
```bash
git submodule update --init --recursive
```

Además, adaptamos un **subconjunto ejecutable** del intérprete ARM1176,
el mapa de memoria y los trampolines BREW de Zeebx a C99
(`cnk_zeebx_arm.[ch]` y `cnk_zeebx_probe.[ch]`).
El código ARM invitado se interpreta dentro de memoria virtual, sin
ejecutarlo como instrucciones nativas de Vita. Los servicios
`malloc` y `IShell::AddRef` tienen respuestas de arranque comprobadas;
las APIs desconocidas detienen el intérprete y generan diagnóstico.

Un análisis **local de referencia**, usando el `cnk2.mod` original privado
del usuario (no incluido), verificó **21185 instrucciones ARM** hasta
el retorno de `AEEMod_Load`, con puntero real a `IModule*`.
No es todavía ejecución de carrera, gráficos BREW o eventos de applet,
ni una prueba del nuevo ejecutable C contra el archivo original en
GitHub Actions. El intérprete original completo de Zeebx no se ha
compilado sin cambios para VitaSDK.

- **24/24 tests portátiles sin ROM aprobados**:
  [GitHub Actions](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38194631197).
- **Compilación ARM de diagnóstico (no instalable)**:
  [VitaSDK](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38194631198).
- [Informe del avance y límites](docs/AVANCE_121_EJECUCION_ZEEBX_ARM_MOD.md).

El modo diagnóstico en Vita se activa únicamente creando el
archivo vacío `ux0:data/cnk3d/zeebx_probe.flag`; lee
`ux0:data/cnk3d/cnk2.mod` localmente y escribe
`ux0:data/cnk3d/zeebx_probe.txt`, **después sigue el mismo motor
nativo**. Por defecto permanece desactivado. No se ha generado
SELF ni VPK, ni se han publicado archivos comerciales del juego.
La parte adaptada de Zeebx está sujeta a su licencia GPL-2.0-or-later.

## Avance 120: lectura del ejecutable BREW gracias a Zeebx

- **Todo el port nativo C existente se conserva.** No se reemplazan
  el motor de carreras, la física, los circuitos ni el renderizador.
- Se adaptaron los lectores de `.mod` y `.mif` de
  [Zeebx](https://github.com/ZeebxTeam/zeebx-emu) a C99,
  con atribución GPL-2.0-or-later en `third_party/zeebx/NOTICE.md`.
- Datos de la copia original privada del usuario: `cnk2.mod` es un
  módulo ARM RAW con salto inicial a `0x14`; `274214.mif` declara
  ClassID `0x01081984`. Los binarios **no** se suben a GitHub.
- Lector independiente `cnk_inspeccionar_mod_zeebx` para explorar
  los archivos originales fuera del repositorio.
- **21/21 pruebas portátiles sin ROM aprobadas**:
  [38193191582](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38193191582).
- **Compilación y enlace ARM VitaSDK aprobados**:
  [38193191572](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38193191572).
- Ver [detalle de desarrollo](docs/AVANCE_120_ZEEBX_BREW_FORMATO_EJECUTABLE.md).

**Importante:** conocer un módulo ARM y su ClassID no significa que
el juego original ya corra en PS Vita. Sigue siendo un port nativo en
desarrollo, sin validación de carrera completa en hardware y sin SELF/VPK.

## Avance 119: primeros sprites originales visibles en Vita

- **Logo original de Crash Nitro Kart 3D:** al seleccionar circuito,
  el código GPU intenta mostrar la imagen real del juego que permanece
  exclusivamente en el `data.vfs` privado del usuario (FUFS 219,
  ID `89b14497`, 326×158, CRC32 `0c1897b1`).
- **Icono original Wumpa:** durante la carrera, junto al contador
  existente del HUD nativo (FUFS 119, ID `43bdcefe`, 32×32,
  CRC32 `5283e7a0`).
- Ambas imágenes usan **magenta opaco como fondo**. El port ahora aplica
  transparencia de clave cromática únicamente a esos PNG comprobados.
  En ROMs que no coincidan, continúa funcionando la interfaz de texto.
- **20/20 pruebas sin ROM en GitHub**:
  [38192490680](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38192490680).
- **Compilación ARM VitaSDK / ELF de diagnóstico**:
  [38192490699](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38192490699).
- Documento: [AVANCE_119_UI_ORIGINAL_PNG_NATIVO](docs/AVANCE_119_UI_ORIGINAL_PNG_NATIVO.md).

Los PNG **no están en GitHub**. Se cargan del FUFS original de la
persona usuaria; no hay VPK ni SELF. La imagen del juego completo,
la jugabilidad en hardware y la fidelidad de las pistas NO se
consideran probadas. La cifra de 45.9 % es solo una estimación histórica.

## Avances 117-118: reutilización real de Zeebx y preparación de texturas

- **Avance 117 (código integrado):** port a C99 del decodificador IMA ADPCM de
  [ZeebxTeam/zeebx-emu](https://github.com/ZeebxTeam/zeebx-emu) (GPL-2.0-or-later).
  `cnk_wav_asset_load` admite ahora WAV PCM y WAV IMA ADPCM a través del motor
  de audio nativo. Se conservan la licencia y el aviso del código adaptado.
  **Los 29 WAV encontrados en el archivo original examinado eran PCM:** el
  códec es soporte adicional, no una recuperación demostrada de audio CNK.
- **Avance 118 (herramienta integrada):**
  `scripts/generar_texturas_desde_cache_zeebx.py` crea filas
  `verified_textures.tsv` compatibles con el renderizador Vita a partir
  de una copia **local, propiedad del usuario** de `data.vfs` y archivos PNG
  originales extraídos por Zeebx. Solo acepta coincidencias de bytes exactas
  y únicas. Sin el caché original externo, ninguna asociación de textura
  nueva se considera verificada.
- **19/19 tests sin ROM en GitHub**:
  [ejecución 38191556688](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38191556688).
- **Compilación y enlace ARM VitaSDK aprobados**:
  [ejecución 38191556755](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38191556755).
- Detalles técnicos:
  [117](docs/AVANCE_117_ZEEBX_DECODIFICADOR_ADPCM.md) y
  [118](docs/AVANCE_118_ZEEBX_COINCIDENCIA_TEXTURAS.md).

**Alcance:** port nativo en desarrollo, NO juego completo ni probado en
PS Vita física. No se ha generado SELF ni VPK, y ningún recurso original
ha sido subido al repositorio. La última cifra de 45.9 % sigue siendo
una **estimación histórica**, no jugabilidad verificada.

## Avance 113 (11 de octubre de 2026)

- Selección de `data.vfs` externo con **verificación ligera de la tabla FUFS**: comprueba encabezado, límites, recuento y referencias antes de elegir una ruta. Evita que un archivo corrupto de mayor prioridad bloquee otra copia válida.
- **132/132 pruebas de PC aprobadas**, incluida la resistencia de carreras; el test FUFS también pasó con ASan/UBSan.
- **13 pruebas automáticas sin ROM** aprobadas en GitHub: [38187660896](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38187660896).
- **Compilación y enlace ARM VitaSDK** aprobados: [38187660904](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38187660904).
- Ver `docs/AVANCE_113_VALIDACION_TABLA_FUFS.md`. **El juego no se ha probado todavía en una PS Vita real, no hay VPK y el porcentaje anterior sigue siendo solo estimado.**

## Avance 112 (11 de octubre de 2026)

- **132/132 pruebas de PC Release aprobadas** con los materiales originales del usuario conservados fuera del repositorio; test del selector externo de FUFS aprobado también con ASan/UBSan.
- **13 pruebas de PC sin ROM aprobadas en GitHub Actions**: [ejecución 38186987038](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38186987038).
- **VitaSDK compiló y enlazó el ELF ARM** con la nueva detección de archivo `data.vfs`: [ejecución 38186987085](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38186987085), commit `b2c2065`.
- El punto de entrada `src/vita_main.c` ya busca el `data.vfs` original del usuario **fuera del futuro VPK**. Ubicación preferida en la consola: `ux0:data/cnk3d/data.vfs`, seguida de `ux0:data/data.vfs` y la ruta heredada `app0:/data.vfs`. Se descartan archivos sin cabecera `FUFS`; después, el motor verifica el archivo completamente al abrirlo.
- Se conservan los avances de telemetría, control y protección de geometría. Ver `docs/AVANCE_111_RUTA_EXTERNA_FUFS.md` y `docs/AVANCE_111_GRILLA_GEOMETRIA_FINITA.md`.
- **Un ELF compilado no es un VPK funcional ni demuestra que una PS Vita real pueda jugarlo.**

## Qué todavía falta

La compilación ARM **no certifica** que arranque el juego en la consola. Siguen pendientes las pruebas en PS Vita física, compatibilidad de gráficos y audio, memoria real, reproducción de todos los recursos y mecánicas originales, optimización y verificación final de jugabilidad. El porcentaje global estimado se conserva en **45.9 %** hasta obtener pruebas en consola.

## Integridad y derechos de datos

Este repositorio contiene **solamente código, herramientas, pruebas y configuración**. No incorporar a GitHub `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROM, música, modelos ni texturas originales. El paquete privado con materiales proporcionados por el usuario se conserva **separado** del código limpio.

## Cómo comprobarlo

Los workflows `.github/workflows/verificacion.yml` y `.github/workflows/vita-elf-link.yml` verifican código en Linux y enlazan el ELF mediante VitaSDK. El segundo ejecuta `scripts/vita_elf_preflight.sh`, audita la arquitectura y archiva el ELF de diagnóstico. **No se genera SELF ni VPK.**
