# Avance 137: GL_FIXED del Zeebo, geometría y UV del juego original

## Cambio integrado

Se preserva el puente original Zeebx `GL_LEGACY` de los avances 129–136, el backend vitaGL real y el motor de carreras nativo existente.

- `glVertexPointer` (slot 78) ahora acepta `GL_FLOAT` (`0x1406`) y `GL_FIXED` (`0x140c`). Las posiciones Zeebo de punto fijo **con signo 16.16** se decodifican como `float` al reconstruir los triángulos del invitado; no se inventa malla ni se ejecuta código comercial directamente como ARM nativo de Vita.
- `glTexCoordPointer` (slot 71) permite esos dos formatos **independientemente**. Una malla de vértices 16.16 puede utilizar UV en coma flotante, o viceversa.
- Se conservan los pasos de 12/16 bytes de XYZ y 8/12 bytes de UV y los límites de lectura de memoria invitada. `DrawArrays` y `DrawElements` siguen usando validación integral antes de avisar al host GPU.
- Un tipo no implementado (`GL_SHORT`, entre otros) sigue provocando parada explícita sin sustituir el puntero, paso ni tipo anterior. Los buffers incompletos y los fallos de la GPU tampoco se reportan como dibujados.

**Fuente de funcionamiento:** ZeebxTeam/zeebx-emu, `src/machine/gl.rs`, lectura de vectores; allí se identificó el soporte de `GL_FIXED`, `GL_FLOAT`, y otras variantes. Este puente únicamente incorpora los formatos de cuatro bytes comprobados aquí, bajo licencia GPL-2.0-or-later.

## Pruebas y limitaciones

`tests/test_native_zeebx_gl_fixed_arrays.c` construye RAM de invitado **sintética** con mallas XYZ fijas, UV fijas y mixtas, pasos intercalados, índices uint16 inversos, errores de formato, direcciones fuera de memoria y rechazo del host GPU. También se actualiza el test de puente original. El flujo automático pasa de **39 a 40 pruebas sin ROM**. La validación VitaSDK usa un ELF diagnóstico sin recursos comerciales.

No se ha probado la carrera completa del juego original en la consola. Continúan pendientes VBOs y offsets, otras rutas GL, la entrada por cuadros del applet, sonido, archivos originales y optimización. **No es un VPK final ni una declaración de jugabilidad**.
