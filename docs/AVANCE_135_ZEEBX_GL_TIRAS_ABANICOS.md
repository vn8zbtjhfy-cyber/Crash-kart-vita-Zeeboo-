# Avance 135: triángulos, tiras y abanicos de Zeebo en la GPU Vita

## Implementación

El ejecutable original Zeebo usa llamadas OpenGL ES Common-Lite a
`glDrawArrays` (slot 26) y `glDrawElements` (slot 27) de `IGL`.
El puente experimental inspirado en Zeebx admitía solamente
`GL_TRIANGLES` (modo 4).

Ahora la misma ruta procesa:

- **`GL_TRIANGLES` (4):** el modo ya existente se conserva.
- **`GL_TRIANGLE_STRIP` (5):** convierte cada tripleta manteniendo
  el sentido frontal. Por ejemplo, 0-1-2-3 pasa a
  0-1-2 / 2-1-3, sin invertir el culling.
- **`GL_TRIANGLE_FAN` (6):** preserva el vértice central:
  0-1-2-3 pasa a 0-1-2 / 0-2-3.

La conversión sucede sobre las instrucciones y datos **originales
de memoria virtual ARM**, no sobre mallas inventadas. Se admiten índices
`GL_UNSIGNED_BYTE` y `GL_UNSIGNED_SHORT` y arreglos secuenciales.
La GPU sigue recibiendo triángulos convencionales por el adaptador
vitaGL ya existente. Se rechazan modos no implementados (líneas,
puntos, etc.).

Todos los vértices y sus índices son validados antes de la llamada GPU:
límites, alineación, posiciones no finitas, conteo y expansión máxima
de **384 vértices por envío**. Si uno falla, no se emite un dibujo
parcial ni se informa éxito.

## Pruebas

- `zeebx_gl_primitives`: orden exacto de caras y vértices, modos
  con/sin índices, Byte y Short, preservación de TRIANGLES,
  desbordamiento y datos ARM malformados.
- `zeebx_gl_bridge`: regresiones anteriores actualizadas: solo
  `GL_LINES` y otros modos no implementados deben quedar detenidos.
- Las **37 pruebas CI sin ROM** revisan el código C de PC.
- El enlace ARM VitaSDK es una validación separada; ninguna
  de estas pruebas reemplaza la consola física.

## Límites

**Todavía NO existe una carrera del juego original validada en PS Vita.**
Faltan texturas y coordenadas UV, audio, acceso completo a archivos,
programación continua de `IApplet::HandleEvent`, temporizadores,
pruebas de hardware y ajuste de rendimiento.

El código adaptado de Zeebx conserva GPL-2.0-or-later. Ningún recurso
comercial, ROM, .mod ni .vfs fue incorporado al repositorio.
El porcentaje global no se eleva sin evidencia física de jugabilidad.
