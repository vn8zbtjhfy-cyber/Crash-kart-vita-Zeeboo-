# Avance 111 - Fuentes originales fuera del VPK

El punto de entrada nativo busca `data.vfs` (FUFS) **por orden**, sin empaquetar
ni copiar los materiales originales:

1. `ux0:data/cnk3d/data.vfs` (ubicación recomendada para el usuario propietario)
2. `ux0:data/data.vfs` (compatibilidad para instalaciones de desarrollo)
3. `app0:/data.vfs` (solo legado: no se distribuye el archivo en un VPK)

Se comprueba únicamente que existan 12 bytes de encabezado y que empiecen
con `FUFS`; `fufs_open()` continúa verificando el contenido completo y rechaza
archivos corruptos. Si no encuentra ninguno, el arranque conserva el código
-1 y escribe un estado en `ux0:data/cnk3d_last_status.txt` si la escritura es
posible. No se descarga ni distribuye la ROM, no se crean materiales sintéticos.

Esta funcionalidad se ha probado con rutas de archivos en Linux. La compatibilidad
de `ux0:` y las rutas de almacenamiento requieren pruebas en una PS Vita física.
