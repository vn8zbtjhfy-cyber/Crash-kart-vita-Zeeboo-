# Avance 120: compatibilidad con el ejecutable original de Zeebo sin perder el port nativo

## Lo que se conserva
**Ningún módulo previo fue reemplazado, eliminado ni reescrito.**
El motor de carreras, los gráficos, la física, los 20 tests existentes,
audio, controles y cargador FUFS siguen siendo el port C99 a Vita.
Ningún `.mod`, `.mif` ni ROM entra en GitHub ni en un VPK.

## Reutilización de código real de Zeebx
Se trasladaron a C99 dos lectores de formato que Zeebx utiliza para
entender ejecutables BREW:
- `src/loader/modfile.rs` → `src/cnk_zeebx_brew.c`, función
  `cnk_brew_mod_inspect`: identifica formato RAW / cabecera BREW,
  extrae offset del punto de entrada y decodifica salto ARM B original.
- `src/loader/miffile.rs` → `cnk_brew_mif_inspect`: analiza las
  secciones de la tabla `.mif` y el identificador original del applet,
  con límites y verificación de entradas.
- `tools/inspeccionar_mod_zeebx.c` (solo PC) proporciona diagnóstico
  directo sin alterar los dos archivos del usuario.

Código fuente original:
https://github.com/ZeebxTeam/zeebx-emu/blob/f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9/src/loader/modfile.rs
https://github.com/ZeebxTeam/zeebx-emu/blob/f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9/src/loader/miffile.rs

**Licencia:** Zeebx GPL-2.0-or-later; ver
`third_party/zeebx/LICENSE` y `NOTICE.md`.
El código fue adaptado; el uso posterior queda sujeto a GPL.

## Hallazgos comprobados en la copia original que el usuario entregó
Un examen local, sin publicar binarios originales, produjo:
- `assets/cnk2.mod`: **649,452 bytes**, SHA-256
  `ccfd648b2d2a30e5abaed582f3b8d26454c59ed0ee34626c14a71ff98337c144`;
  variante RAW (no cabecera `BREW` en offset 8).
- La primera instrucción ARM es `0xEA000003`, que salta del offset
  0 al **0x14**. Ese 0x14 es un destino dentro del ejecutable BREW,
  **no una función nativa que se pueda llamar directamente desde Vita**.
- `assets/274214.mif`: contiene **10 secciones**; el registro
  de applet principal contiene ClassID **`0x01081984`**.
- La cabecera BREW original, el ClassID y los offsets pueden usarse
  para comprobar la identidad de binarios al comparar trazas futuras
  de Zeebx con las rutinas C del port.

En una PC Linux/macOS (y mediante compilación equivalente en Windows):
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target cnk_inspeccionar_mod_zeebx
./build/cnk_inspeccionar_mod_zeebx /ruta/cnk2.mod /ruta/274214.mif
```

## Alcance real y límite importante
**Un analizador de ejecutable NO es una CPU virtual ni ejecuta el juego.**
Zeebx ejecuta el .mod mediante un núcleo ARM (Dynarmic o interpretador
A32/Thumb), memoria virtual guest, sistema BREW y rutas de API,
renderización, audio y entrada. No se puede sustituir eso solo
copiando un archivo Rust al port C99/VitaSDK. Aun así,
esta adaptación integra información del emulador en el mismo código C
del port sin perder la implementación nativa ya construida.

La siguiente tarea técnica para usar aún más Zeebx es obtener
trazas concretas de ejecución de `cnk2.mod` en el emulador y
compararlas con los ciclos de inicialización del motor Vita.
No se afirma una ejecución real de `cnk2.mod` en PS Vita.
No se genera ELF instalable, SELF ni VPK.
