# Crash Nitro Kart 3D | nuevo port Rust/Vita (experimental)
Este arbol separado contiene el puente Zeebx+Rust, sin modificar el port C original de la raiz.
El VPK no se considera jugable ni verificado. El workflow busca generar
Crash_Nitro_Kart_3D.vpk solo cuando compile la biblioteca Rust y existan TODOS
los recursos RomFS embebidos. No sustituir los datos faltantes con archivos vacios.
El ZIP local del usuario tiene 35 archivos. Los binarios comerciales no se han
podido subir con la conexion GitHub de texto, por lo que esta rama aun NO es
autosuficiente. La ejecucion de Actions identificara el primer bloqueo real.

## Pruebas en consola
Las pruebas de Linux con la interfaz Rust original verifican el menú y una escena
de carrera; **no garantizan rendimiento ni ausencia de bloqueos en una Vita real**.
Las comprobaciones de escena `R.scene.*` permanecen activas, pero la escritura
de dos capturas PPM de casi 1 MiB cada una está **desactivada por defecto** para
no introducir pausas de almacenamiento durante la partida. Para habilitarla
en una prueba de diagnóstico, crea un archivo vacío llamado
`ux0:data/cnk_vita_capture.enable` con VitaShell **antes de ejecutar el juego**.
Los archivos capturados serán `ux0:data/cnk_vita_menu_25s.ppm` y
`ux0:data/cnk_vita_race_40s.ppm`. Eliminar el archivo indicador desactiva las
capturas en el siguiente arranque. Un error de textura de vitaGL ahora se
registra en `ux0:data/cnk_vita_boot.log` como `C.video.error.code` y se
detiene la presentación, en vez de continuar silenciosamente con datos no válidos.

## Modo rendimiento y salida de bloqueos

El VPK inicia ahora el rasterizador **3D por software a 1/4 de la resolución
interna por eje**. El juego sigue presentando su imagen RGB565 original a
640×480. Esto reduce el trabajo de rellenado 3D, a cambio de bordes más
pixelados; no acelera las instrucciones ARM interpretadas por sí mismas.
Si se desea comparar la calidad anterior, crear con VitaShell el archivo vacío
`ux0:data/cnk_vita_quality.enable` y reiniciar el juego: habilita reducción 2x.
Eliminar ese archivo y reiniciar restaura el modo rendimiento 4x.

El puente no vuelve a subir a vitaGL los fotogramas idénticos, pero continúa
avanzando los eventos y temporizadores del juego. Las llamadas ARM también
se dividen en bloques más cortos, facilitando leer los botones entre pasos.
Para salir del VPK, mantener **START + SELECT** hasta que el bucle principal
vuelva a consultar el mando. Si una llamada nativa del rasterizador quedara
atrapada, este atajo no sustituye el menú de sistema de PS Vita.

**Importante:** la prueba automatizada se ejecuta en Linux y comprueba
regresiones de menú/carrera con los mismos métodos Rust, pero no mide FPS
ni descarta cuelgues en una PS Vita real. Registrar una prueba en la consola
antes de anunciar mejoras medidas de fluidez.

## Prueba de menú y carrera en una PS Vita real (sin depender del mando)

Para diagnosticar si el problema es la emulación o el adaptador de controles,
crear un archivo **vacío** con VitaShell en
`ux0:data/cnk_vita_autotest.enable` **antes de abrir el juego**.
Al arrancar, el puente reproduce la secuencia física equivalente a
Cross a los 35, 60 y 90 segundos de tiempo **emulado** y movimientos
de stick a los 70-85 y 95-110 segundos, la misma secuencia que alcanzó
menú y carrera en el test de Linux. **No sustituye imágenes ni fabrica
pantallas de juego.** El reloj emulado no equivale al tiempo real
en una Vita, así que no promete que el menú aparezca en 20 segundos reales.

El registro `ux0:data/cnk_vita_boot.log` incluirá
`R.autotest.controls.enabled` al arrancar. Tras reconocer
los fotogramas originales (heurísticas de color), registrará
`R.scene.menu.verified_live` y
`R.scene.race.verified_live`. Si la pantalla queda negra,
copia ese registro junto con la captura del error. Esos mensajes
no prueban los FPS ni la fluidez de la consola.
**Borra `ux0:data/cnk_vita_autotest.enable` para jugar con tus propios
controles:** con ese archivo presente, la prueba controla el mando.
El archivo solo se lee al iniciar el juego.
