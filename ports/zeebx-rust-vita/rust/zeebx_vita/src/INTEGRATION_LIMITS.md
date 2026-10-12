# Límites verificados

Este código no está certificado como compilable/ejecutable en PS Vita.
El repositorio no contiene aún los doce blobs comerciales de `assets/manifest.json`.
GitHub Actions puede fallar al adaptar Zeebx a Vita, por dependencias no soportadas.
Aunque el compilador terminase, un VPK no demostraría jugabilidad. No presentar una prueba de enlace como juego terminado.

Los 412 IDs del contenedor FUFS se mantienen en memoria: el recurso `data.vfs` se pasa íntegro a BREW. No se asignan rutas hipotéticas a dichos IDs.
