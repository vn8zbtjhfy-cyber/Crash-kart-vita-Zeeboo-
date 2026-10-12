# Avance 130: matriz y transformaciones 3D del GL original

Se ampliaron los puentes de Zeebx GL_LEGACY hacia vitaGL. Los indices de
funcion proceden de ZeebxTeam/zeebx-emu, revision f6d3e7bd:

- 46 glLoadIdentity
- 51 glMatrixMode (modelview, projection, texture)
- 60 glPopMatrix; 61 glPushMatrix
- 63 glRotatex; 65 glScalex; 77 glTranslatex

Los argumentos de geometria usan numeros 16.16 con signo, convertidos a
float antes de enviarlos a las funciones de matriz de vitaGL. No hay
un simulador ficticio de pila de matrices: la GPU recibe las operaciones.
Se comprueban errores del renderizador y los modos de matriz invalidos
detienen la ejecucion del applet. Las llamadas requieren EGL activo.

La prueba sintetica test_native_zeebx_gl_bridge valida los indices y
argumentos, el signo negativo, el fallo del backend, la falta de contexto
y la conservacion del bloqueo para glDrawElements.

No incluye assets de Crash ni demuestra una carrera jugable: faltan
geometria, texturas, estados, flujo continuo del applet, audio y
verificacion en consola. El objetivo permanece como unica instalacion
VPK privada con recursos propios, sin pedir carpetas al usuario.
