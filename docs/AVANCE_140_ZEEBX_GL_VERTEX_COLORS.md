# Avance 140: colores por vértice originales de Zeebx en vitaGL

## Qué se porta

- BREW `GL_LEGACY` slot 14, `glColorPointer`, que Zeebx implementa con la matriz de atributos OpenGL ES. Soporta por ahora `GL_UNSIGNED_BYTE` de 4 componentes RGBA, normalizado dividiendo cada canal entre 255, equivalente al algoritmo `src/machine/gl.rs`.
- `glEnableClientState(GL_COLOR_ARRAY)` y `glDisableClientState(GL_COLOR_ARRAY)` mantienen el estado de color independiente de vértices y coordenadas de textura.
- Se soportan posiciones de color dentro de la RAM virtual del juego y dentro de VBOs (incluidos offsets de bytes y `stride` para intercalados) con los mismos límites de memoria que XYZ/UV.
- Se agregan callbacks para enviar al GPU Vita triángulos con **color por vértice**, solos o con textura. No se usan punteros sin validar del juego; la conversión termina antes de llamar a `glColor4f`, `glTexCoord2f` y `glVertex3f` del host.
- Datos desconocidos, punteros incompletos y errores de la GPU detienen la llamada BREW, sin atribuir a Crash un fotograma exitoso que no se dibujó.

## Comprobación

- `tests/test_native_zeebx_gl_colors.c` usa datos originales **sintéticos**: RGBA de 8 bits con alfa diferente entre vértices, indexación inversa, memoria invitada y VBO intercalado, faltas de búfer, textura deshabilitada y rechazo de GPU.
- Workflow de PC ampliado a **43 pruebas sin ROM** y diagnóstico VitaSDK ELF en rama de desarrollo. Un resultado verde confirma compatibilidad del código sometido a prueba, **no** una carrera en la PS Vita.

## Estado y límites

El port nativo con ejecución específica del juego **no se encuentra completo**. En particular no está probado el bucle del juego original con sus recursos y sonido durante una carrera entera en una PS Vita física. Esta implementación se limita a una API real de render del juego, no es un VPK terminado ni una demostración de jugabilidad. Sin originales comerciales ni ROM en GitHub.

Referencia: https://github.com/ZeebxTeam/zeebx-emu/blob/f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9/src/machine/gl.rs (GPL-2.0-or-later).
