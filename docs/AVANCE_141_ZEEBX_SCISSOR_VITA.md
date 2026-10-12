# Avance 141: recorte 2D original del Zeebo con vitaGL

## Implementación comprobable

El puente de ejecución del ejecutable BREW original ahora reconoce `IGL::glScissor` en el **slot 66** de `GL_LEGACY`, utilizando la tabla de `ZeebxTeam/zeebx-emu/src/brew/aee_slots.rs`. También acepta habilitar/deshabilitar `GL_SCISSOR_TEST` (0x0C11) mediante los slots 28/24.

La llamada de recorte se valida antes de llegar a la GPU: anchura y altura no pueden ser negativas, coordenadas deben estar dentro de límites finitos conservadores y el contexto EGL debe estar activo. Sin el backend del host, ante límites inválidos o errores devueltos por vitaGL, el intérprete **se detiene y no inventa éxito**.

`src/cnk_vita_zeebx_gl.c` transforma las coordenadas del lienzo original **640×480** al área centrada **725×544** de la pantalla de Vita **960×544**. El recorte sigue la misma escala y origen que `glViewport` y no invade las barras laterales. Ejemplo de referencia: `(160,120,320,240)` pasa a `(298,136,362,272)`, sujeto a redondeo entero del adaptador. El backend llama **`glScissor` real** y comprueba `glGetError`.

Pruebas sin ROM:
- `zeebx_gl_state`: registro de cambios de `GL_SCISSOR_TEST`, recorte del huésped, dimensiones negativas, callback ausente y fallo del GPU.
- `zeebx_vitagl_projection`: transformación espacial y propagación del error real del renderizador, mediante un stub host (no una Vita física).

## Limitaciones

Estos cambios **no prueban todavía que el juego original sea jugable**. Falta completar más funciones 3D y EGL/GL, ejecución continua del applet, recursos, audio, pruebas con ROM privada en el dispositivo y medición de rendimiento. El intérprete derivado de Zeebx conserva licencia GPL-2.0-or-later. No se incluye software comercial en el repositorio.

