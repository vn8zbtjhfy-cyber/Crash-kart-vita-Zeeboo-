# Avance 129: primeras llamadas gráficas del Zeebo llegan a vitaGL

## Implementación

- IEGL antiguo de Zeebx (AEEGL.h) crea una superficie de ventana y contexto real solo cuando hay callbacks de GPU registrados. Sin backend, las funciones siguen deteniéndose con diagnóstico.
- eglMakeCurrent, eglGetCurrentContext, eglGetCurrentSurface y eglSwapBuffers comprueban que la superficie y el contexto sean válidos. SwapBuffers solo indica éxito después de que el callback de presentación del host responde éxito.
- IGL antiguo, sin parámetro this, traduce glClear (slot 7), glClearColorx (slot 8, punto fijo 16.16), glGetError (slot 37) y glViewport (slot 79). Las funciones no implementadas, incluidas DrawArrays, DrawElements, texturas y matrices, se detienen sin inventar éxito.
- El nuevo src/cnk_vita_zeebx_gl.c redirige estas operaciones a la GPU mediante vitaGL. Se respeta la relación 4:3 del Zeebo en la pantalla 960x544 de Vita.
- El sondeo original usa archivos privados del usuario y se ejecuta **después** de vglInit dentro del runtime Vita, nunca antes de inicializar la GPU.
- El informe del sondeo anota gpu_host_bound, guest_gl_clear_calls, guest_gl_viewports y guest_gl_presented_frames. El modo nativo anterior no se sustituye.

## Qué demuestran las pruebas

- El test nuevo tests/test_native_zeebx_gl_bridge.c usa un backend de dibujo registrador y comprueba estado EGL, colores 16.16, limpieza de pantalla, viewport y presentación.
- Comprueba que sin renderizador, con contexto inactivo o cuando la GPU rechaza una operación no se devuelva un éxito falso.
- Se ejecutan 33 pruebas sin ROM en CI. La compilación para VitaSDK es separada. Esto no equivale a haber probado una carrera en hardware.

## Bloqueos restantes

1. Portar geometría, texturas, estados fijos y matrices del GL ES original mediante Zeebx y vitaGL.
2. Procesar IApplet::HandleEvent, timers, audio y datos de archivos en el flujo original por cuadro.
3. Probar en una PS Vita física el menú y una carrera del juego original, corregir errores y evaluar rendimiento.

No es un VPK final ni una confirmación de jugabilidad. No se incorporaron ROM, MOD, MIF, música ni recursos comerciales al repositorio.
La adaptación de código Zeebx conserva GPL-2.0-or-later.
