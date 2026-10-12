# Avance 143: atlas/texturas parciales originales de Zeebo en Vita

## Qué cambió realmente

Se utiliza la definición original del emulador **Zeebx**, en `src/brew/aee_slots.rs` (GL_LEGACY 57/74/76) y `src/machine/gl.rs::le_texels/gles_tex_sub_image`. No se incorpora ROM ni se imitan imágenes.

- `glPixelStorei(GL_UNPACK_ALIGNMENT)` acepta 1, 2, 4 u 8. El valor predeterminado, como GLES1/Zeebx, es 4. No altera GPU si el parámetro del invitado es inválido.
- `glTexImage2D` y `glTexSubImage2D` leen los píxeles **de memoria ARM virtual**. Al tener ancho de fila impar, respetan los bytes de relleno para no deformar la textura. Preparan una copia compacta antes de llamar a VitaGL.
- `glTexSubImage2D` (slot 76, parámetros AAPCS en r0-r3 y cinco argumentos en la pila) actualiza sólo el rectángulo indicado de la textura ya existente. El backend real llama a `glTexSubImage2D`; un error de GPU detiene la API del invitado.
- Se valida tamaño, formato, tipo, alineación, rectángulo, dirección y longitud de la copia. El máximo de 512x512 y los formatos RGB565/RGBA8 siguen siendo conservadores, por lo que aún hay estados/formats no implementados.

## Evidencia y límites

`tests/test_native_zeebx_subimage.c` crea un dispositivo invitado sintético, prueba RGB565 de ancho impar, la ausencia de desplazamiento de filas, lecturas fuera de límites, offsets negativos y errores del backend. `tests/test_native_zeebx_vitagl_textures.c` comprueba llamadas reales al adaptador usando un registrador GL de PC. GitHub Actions compila separadamente el puente VitaSDK.

**Esto NO demuestra que el videojuego original se vea o se juegue en Vita.** Siguen faltando llamadas GLES del juego, el ciclo original continuo `IApplet::HandleEvent`, acceso a datos y audio, los recursos del propietario y pruebas físicas de carrera. No se comparte ningún contenido original de Crash. La adaptación de Zeebx mantiene obligaciones GPL-2.0-or-later.
