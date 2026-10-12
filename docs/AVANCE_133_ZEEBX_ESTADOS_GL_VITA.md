# Avance 133: estado gráfico original de Zeebx en vitaGL

## Qué se integró realmente

Desde la tabla de slots `GL_LEGACY` del Zeebx anclado en el submódulo, se conectaron las llamadas BREW originales de **Crash Nitro Kart 3D** para que modifiquen el contexto de **vitaGL de PS Vita**, mediante el puente `cnk_zeebx_gl_host`:

- `glEnable` / `glDisable` para `GL_DEPTH_TEST`, `GL_BLEND` y `GL_CULL_FACE`.
- `glBlendFunc` con factores básicos de transparencia `ZERO`, `ONE`, `SRC_ALPHA` y `ONE_MINUS_SRC_ALPHA`.
- `glDepthFunc` y `glDepthMask` para comparación y escritura de profundidad.
- `glCullFace` y `glFrontFace` para descarte de caras y orientación.
- `glColor4x` con normalización del punto fijo Zeebo 16.16 y `glColorMask` para selección de canales.

Todas pasan por callbacks de **GPU real**; sin renderer, sin contexto EGL, con enumeraciones inválidas o cuando falla `glGetError`, el intérprete **se detiene en la función no soportada**. No se retornan éxitos inventados.

Se **rechaza** `glEnable(GL_TEXTURE_2D)` deliberadamente hasta que existan realmente enlaces de texturas, coordenadas UV y cargas `glTexImage2D` desde los archivos del juego. Esto evita que un triángulo no texturizado se presente como una imagen fiel del título.

## Verificación y alcance

- Nuevo test `zeebx_gl_state` sin ROM, junto con la suite original sin datos comerciales. Verifica aceptación y rechazo de estados GL, valores transportados al host, punto fijo, callbacks ausentes y fallos del GPU.
- El código real `src/cnk_vita_zeebx_gl.c` se compila en la prueba de ciclo Vita usando stubs y en el enlace con VitaSDK, pero **la Vita física aún no ha sido probada**.
- La ejecución original del applet sigue siendo **diagnóstica y limitada**, no una carrera reproducida. Todavía quedan geometría texturizada, matrices adicionales, archivo/sistema de juego, audio, eventos por cuadro y pruebas físicas.
- La adaptación conserva la atribución **GPL-2.0-or-later** de Zeebx. No incluye ROM, `data.vfs`, `cnk2.mod`, música, texturas ni otros archivos comerciales.

Este avance mejora el puente **original Zeebo → Vita**, no supone que el juego esté terminado ni aumenta automáticamente su porcentaje estimado de jugabilidad.
