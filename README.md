# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** del port nativo de Zeebo a PlayStation Vita. **Todavía no es un videojuego completo ni se ha verificado su ejecución en consola.**

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
