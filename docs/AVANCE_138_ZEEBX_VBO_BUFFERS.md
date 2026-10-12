# Avance 138: buffers de geometría originales Zeebx para Vita

## Qué se adaptó

Siguiendo `ZeebxTeam/zeebx-emu/src/machine/gl.rs` y su tabla `src/brew/aee_slots.rs`, se añadieron las siguientes llamadas **GL_LEGACY de Zeebo** al puente original de Vita:

- `glBindBuffer` (119), `glBufferData` (120), `glBufferSubData` (121), `glDeleteBuffers` (124), `glGenBuffers` (128) y `glIsBuffer` (137).
- Soporte para `GL_ARRAY_BUFFER` (0x8892) y `GL_ELEMENT_ARRAY_BUFFER` (0x8893). Los nombres generados no son buffers vivos hasta que se enlazan; se evitan colisiones con nombres que el juego ya haya enlazado.
- `glVertexPointer` y `glTexCoordPointer` conservan el buffer vinculado **en el momento de configurar el vector**; por eso aceptan puntero `0` como **desplazamiento cero dentro del VBO**, y no como una dirección ARM inválida. Los índices de `glDrawElements` usan el `ELEMENT_ARRAY_BUFFER` **actual**, igual que Zeebx.
- Se conservan los soportes anteriores de vértices `GL_FIXED` 16.16 y `GL_FLOAT`, UV, triángulos indexados, tiras y abanicos; se expanden a valores `float` temporales antes de llamar al GPU real mediante vitaGL. **No se inventa geometría ni se ejecuta código de Zeebo como instrucciones nativas de Vita.**
- `BufferData` permite origen nulo e inicialización vacía seguida de `BufferSubData`. Los buffers se limitan a 32 objetos, máximo 512 KiB por objeto y 2 MiB en conjunto. Accesos fuera de rango, tipos no soportados y fallos de host se detienen explícitamente.

## Verificaciones

Una prueba **sin ROM** fabrica bytes BREW en RAM invitada y recorre `GenBuffers → BindBuffer → BufferData → BufferSubData → Pointer(offset=0) → DrawElements` con UV originales 16.16 y referencias a otro buffer. Verifica el resultado en el mismo callback utilizado para la adaptación de Vita y los fallos de tamaño/índice, objeto borrado y memoria. La batería CI debe pasar de **40 a 41 pruebas**. La compilación VitaSDK enlaza por separado el ELF ARM.

## Lo que sigue pendiente

No hay carrera original comprobada en PS Vita física. Quedan funciones GL de Zeebx, secuenciación de `IApplet::HandleEvent`, lectura de archivos originales, sonido y pruebas de rendimiento. El soporte de VBO aquí **no crea un VPK final**. No se incorporan ROM ni recursos comerciales al repositorio. La reutilización del código Zeebx conserva GPL-2.0-or-later.
