# Avance 142: GL_REPLACE de Crash mediante Zeebx y vitaGL

## Qué se hizo

El proyecto conserva el motor nativo que ya existía. El puente aislado de ejecución de `cnk2.mod` ahora traduce dos funciones de la **interfaz IGL original**, siguiendo el orden de funciones de `ZeebxTeam/zeebx-emu/src/brew/aee_slots.rs`:

- Slot 72, `glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, valor)`.
- Slot 73, `glTexEnvxv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, puntero_guest)`.

La fuente de Zeebx (`src/machine/gl.rs`) anota expresamente que **Crash utiliza la forma vectorial para solicitar `GL_REPLACE`**. El modo de textura es una enumeración OpenGL cruda incluso al usar las funciones `x`; no se divide entre 65,536.

Se admiten solamente los modos básicos de GLES1: `GL_MODULATE`, `GL_REPLACE`, `GL_DECAL`, `GL_BLEND` y `GL_ADD`. La lectura vectorial verifica la alineación, los límites y que el puntero pertenezca a memoria virtual de Zeebo antes de pasar el **valor**, nunca el puntero, a la GPU. Las combinaciones, modos o parámetros no implementados detienen el diagnóstico de la API; tampoco se informa éxito si falta la GPU o si `glGetError` falla.

El adaptador `src/cnk_vita_zeebx_gl.c` conecta de forma directa el callback con `glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode)` de vitaGL, después de inicializar la GPU.

## Verificación sin materiales comerciales

- `tests/test_native_zeebx_texture_env.c`: entorno EGL real del puente, llamadas escalares y vectoriales, punteros inválidos, enumeraciones desconocidas, callback ausente y GPU que rechaza la operación.
- `tests/test_native_zeebx_vitagl_textures.c`: el adaptador de producción compilado contra un registrador de llamadas GL prueba que cambia entre `GL_REPLACE` y `GL_MODULATE`, e informa errores reales del backend.
- GitHub Actions ejecuta las pruebas con RAM guest sintética, nunca con ROM de Crash. VitaSDK se enlaza mediante el workflow separado al integrar en `main`.

## Límites

La adaptación de este estado de texturas **no equivale a un menú o carrera original ya jugable**. El intérprete Zeebx todavía necesita resolver otras llamadas GL, eventos continuos de IApplet, carga de archivos, audio y verificar las texturas completas con el propietario de los datos y una PS Vita física. No se incluyeron `cnk2.mod`, `274214.mif`, `data.vfs` ni recursos comerciales.

Código Zeebx adaptado bajo GPL-2.0-or-later con sus avisos existentes.
