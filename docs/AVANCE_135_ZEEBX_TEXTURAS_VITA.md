# Avance 135: texturas originales Zeebo hacia la GPU de Vita (experimental)

## Qué está implementado

Se adaptaron funciones verificadas contra `ZeebxTeam/zeebx-emu/src/brew/aee_slots.rs` y `src/machine/gl.rs` a la ejecución BREW ARM invitada y al backend vitaGL:

- `glGenTextures` (slot 36), `glDeleteTextures` (20), `glBindTexture` (5), `glTexParameterx` (75) y `glTexImage2D` (74) reciben argumentos reales. `glTexImage2D` lee sus **cinco parámetros restantes de la pila ARM invitada**, no del stack de Vita.
- La carga se limita de momento al nivel cero de `GL_RGBA / GL_UNSIGNED_BYTE` y `GL_RGB / GL_UNSIGNED_SHORT_5_6_5`, máximo 512×512. Las texturas se copian temporalmente fuera de la RAM invitada antes de llamar al driver. Dirección, tamaño, desbordamiento, alineación y argumentos inválidos bloquean la llamada **sin confirmar éxito**.
- `glTexCoordPointer` (71) acepta UV de dos floats con stride validado. `GL_TEXTURE_COORD_ARRAY` habilita el dato cliente; `GL_TEXTURE_2D` solo se activa cuando existe un host de texturas real. `DrawArrays` y `DrawElements` alimentan el renderer con vértices XYZ y UV reales, o se detienen si faltan recursos válidos. El dibujado sin textura queda conservado.
- La GPU Vita recibe `glGenTextures`, `glBindTexture`, `glTexImage2D`, `glTexParameteri`, `glDeleteTextures` y `glTexCoord2f` a través del adaptador real. Sus errores llegan a la máquina invitada.
- La comparación con la lista original de `GL_LEGACY` detectó y corrigió offsets anteriores de `glDepthFunc` (slot 21), `glDepthMask` (22) y `glDisable` (24). Se ajustaron los tests que habían codificado ese error.

## Cómo se comprueba

Dos tests **sin ROM** reproducen llamadas GL del código invitado y el adaptador de producción usando un grabador falso de OpenGL. Comprueban formatos de textura, orden de UV, presión sobre memoria fuera de límites y rechazo explícito de fallos GPU. La validación ARM de VitaSDK es una compilación cruzada y enlace, nunca prueba real en una consola.

## Bloqueos existentes

**Aún no está terminado ni es jugable el juego original.** Faltan otras variantes GL (texturas comprimidas ATC, mipmaps y efectos), archivos, shaders/matrices/estado adicional, eventos IApplet por cuadro, sonido, pruebas físicas de una carrera completa, rendimiento y correcciones. No hay binarios ni `data.vfs` originales incluidos. Los componentes adaptados de Zeebx conservan sus condiciones GPL-2.0-or-later.

**El usuario final sigue teniendo la meta de instalar una sola burbuja y jugar**, pero este bloque no declara que esa meta ya se haya alcanzado.
