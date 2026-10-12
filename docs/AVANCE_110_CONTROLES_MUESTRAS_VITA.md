# Avance 110: tolerancia a muestras vacías del mando PS Vita

Fecha: 11 de octubre de 2026.

La API no bloqueante `sceCtrlPeekBufferPositive` puede devolver cero cuando no hay una nueva muestra. Antes, una sola lectura vacía hacía salir del juego (`-4`). Ahora:

- Hasta **siete** muestras vacías consecutivas producen entrada neutral (sin acelerar, dirigir, usar ítems ni confirmar menús).
- La **octava** lectura vacía consecutiva sigue devolviendo un fallo para no ocultar una desconexión o controlador permanentemente sin respuesta.
- Un error negativo real de la API termina inmediatamente con el mismo resultado de error.
- Una muestra válida reinicia el contador; la inicialización lo reinicia también.

El test host `vita_controls` revisa ambas rutas y la recuperación tras una muestra válida. **130/130** pruebas de PC Release aprobadas. No demuestra latencia, jugabilidad ni arranque real en la consola.

No se ha generado un VPK ni aumentado la estimación histórica del 45.9 %.
