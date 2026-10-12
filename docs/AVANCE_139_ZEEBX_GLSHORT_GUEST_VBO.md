# Avance 139: coordenadas GL_SHORT reales de Zeebx para Vita

## Implementación en el juego original

- `src/cnk_zeebx_arm.c` acepta `GL_SHORT=0x1402` en los métodos originales `glVertexPointer` y `glTexCoordPointer` de IGL antiguo (BREW Zeebo).
- El código adapta **valores i16 con signo y little-endian** a flotantes en **unidades de modelo**, siguiendo `ZeebxTeam/zeebx-emu/src/machine/gl.rs::read_attribute`. **GL_SHORT NO se divide por 65536**. `GL_FIXED` sigue siendo 16.16; `GL_FLOAT` sigue siendo IEEE 754.
- Soporta los tres recorridos: datos originales en la memoria virtual del juego, `DrawArrays` y `DrawElements`, y búferes VBO/IBO con **desplazamiento de bytes no cero**. El estado original del búfer se conserva al desvincularlo, según semántica GLES.
- `stride=0` se traduce a 6 bytes/XYZ y 4 bytes/UV, no 12 y 8. El código admite los strides explícitos intercalados, con dos bytes por componente.
- Se mantienen los límites de vértices, el control de desbordamiento mediante direcciones de 64 bits y la parada ante un `GL` desconocido, puntero no alineado, memoria incompleta o GPU que rechaza el dibujo. Nada de éxitos inventados.

## Pruebas

- Se agregó `tests/test_native_zeebx_gl_short.c`, sin ROM, con valores i16 extremos `-32768`/`32767`, XYZ/UV intercalados y compactos, triángulos indexados al revés, VBO/IBO con offsets 10/70/2, mal alineamiento y memoria inexistente, y rechazo real de la callback de GPU.
- También se actualizaron dos pruebas anteriores para que `GL_BYTE` continúe siendo un formato explícitamente no soportado, ahora que `GL_SHORT` está portado.
- CI verifica 42 pruebas sin ROM y el enlace ARM VitaSDK por separado. Hasta obtener resultados de Actions, estos son criterios de validación, no resultados asegurados.

## Límite real

Esta incorporación **no demuestra todavía que el juego original arranque o termine una carrera en una Vita física**. Se siguen necesitando datos originales del usuario, todos los métodos GL/BREW que use Crash, flujo del applet, manejo de sonido/archivos y la prueba real en consola. Un enlace ELF correcto no equivale a juego jugable. No hay ROM, música ni VPK final en este commit.

Referencia de API: [Zeebx gl.rs](https://github.com/ZeebxTeam/zeebx-emu/blob/f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9/src/machine/gl.rs). Adaptación de código bajo GPL-2.0-or-later.
