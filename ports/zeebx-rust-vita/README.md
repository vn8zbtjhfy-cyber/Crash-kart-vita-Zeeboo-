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
