# Avance 115: interfaz de carrera compacta, inspirada en referencias de Zeebo

El gameplay compartido por el usuario (https://youtu.be/HaFwpnlci2Y) y las capturas públicas de la versión Zeebo motivan revisar la distribución de los datos de carrera. No se ha realizado todavía una medición fotograma por fotograma ni se afirma equivalencia visual con la interfaz original.

## Cambio implementado

- En estado RACE se deja de dibujar la barra de diagnóstico que cubría el ancho superior e inferior de la pista; SELECT, PAUSE y RESULTS conservan la pantalla de desarrollo.
- Los datos del motor nativo se muestran ahora en regiones compactas: tiempo y Wumpa arriba a la derecha, posición y vuelta abajo a la izquierda, minimapa existente abajo a la derecha y turbo abajo en el centro.
- Las señales 3-2-1-GO, contra dirección y aviso de proyectil siguen disponibles.
- `cnk_race_hud_text_build` centraliza el reloj MM:SS:centésimas, la conversión correcta de vueltas terminadas a vuelta actual y el formato de posición/Wumpa, con validación antes de escribir.
- El medidor de turbo conserva los estados reales de la simulación nativa; ninguna cifra de duración o física de Zeebo se atribuye como recuperada.

## Restricciones de fidelidad

Esta NO es la interfaz, tipografía ni un sprite original de Zeebo. No se agregaron ni copiaron texturas, música, ROM, `data.vfs` ni assets de Infuse. La licencia de https://github.com/Tuxality/Infuse impide empaquetar su aplicación sin permiso, y el árbol público de ese repositorio no incluye su código fuente. Infuse sirve como referencia externa de ejecución, no como parte de un port nativo.

Prueba `race_hud_text` sin ROM para reloj, posición, vuelta y errores. Validación de PC y enlace ARM VitaSDK pendiente al redactar este archivo. No hay pruebas en consola física, no hay porcentajes nuevos y no se crea SELF ni VPK.
