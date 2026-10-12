# Avance 144: estados GLES1 del juego original, tomados de Zeebx

## Objetivo y alcance

Adaptar **únicamente** las interfaces del juego Crash Nitro Kart 3D desde Zeebx, sin reproducir ni mostrar interfaz, catálogo o menú de un emulador de computadora. Mantener los módulos nativos de Vita existentes y el puente experimental del archivo `cnk2.mod` aislado. Esto sigue siendo un intérprete ARM/BREW de compatibilidad específico del juego, no prueba de ejecución totalmente nativa de su binario de Zeebo.

La correspondencia de funciones viene de `ZeebxTeam/zeebx-emu/src/brew/aee_slots.rs::GL_LEGACY`, GPL-2.0-or-later. Se requieren los avisos correspondientes para distribuir cualquier port que adapte ese código.

## Operaciones nuevas

- Slot **4** `glAlphaFuncx`: prueba alfa GLES1, `GL_NEVER` a `GL_ALWAYS`, umbral signed 16.16 transformado a float entre 0 y 1 y llamada auténtica a `glAlphaFunc` del backend vitaGL.
- Slot **59** `glPolygonOffsetx`: factor y unidades signed 16.16, enviados a `glPolygonOffset` en Vita.
- Slot **67** `glShadeModel`: acepta exclusivamente `GL_FLAT` y `GL_SMOOTH` y los aplica al renderizador Vita.
- Slots **24/28** `glDisable/glEnable`: se agregan las capacidades `GL_ALPHA_TEST` y `GL_POLYGON_OFFSET_FILL` del GLES1 original, dejando prohibidos los identificadores no reconocidos.

Las funciones NO devuelven éxito si falta el callback, no hay contexto EGL, la enumeración es inválida o la GPU comunica error.

## Evidencia y límites

- `tests/test_native_zeebx_gl_state.c` verifica la ABI del invitado, valores numéricos, transiciones de estado, rechazo por callback ausente y errores del host.
- `tests/test_native_zeebx_vitagl_projection.c` verifica que el adaptador real emita operaciones GL bajo un registrador de PC, incluidos fallos informados por `glGetError`.
- `tests/stub/vitaGL_stub.c` es solo código de prueba. La aplicación Vita usa `src/cnk_vita_zeebx_gl.c` y las bibliotecas reales de VitaSDK.

El objetivo final sigue siendo **una burbuja única, iniciar Crash directamente y jugar**, sin configurar un emulador. Por ahora todavía faltan otras APIs de Zeebx, el bucle auténtico `IApplet::HandleEvent` y la integración de datos, audio y carrera real en la consola. Ninguna ROM ni recursos comerciales se incorporan al repositorio.