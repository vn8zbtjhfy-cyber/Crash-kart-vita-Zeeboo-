# Avance 148: metadatos de archivos originales BREW de Zeebx

## Cambio real integrado

Se portaron de `ZeebxTeam/zeebx-emu/src/machine/file.rs` y su tabla
`src/brew/aee_slots.rs` dos llamadas BREW que faltaban al ejecutar
`cnk2.mod` (código adaptado bajo GPL-2.0-or-later):

- **IFileMgr::GetInfo (interfaz 3, slot 3)**: consulta un recurso
  de la carpeta privada del propio juego, sin abrirlo permanentemente.
- **IFile::GetInfo (interfaz 4, slot 6)**: consulta un archivo ya abierto
  sin mover su cursor ni consumir bytes.

Ambas escriben **AEEFileInfo de 76 bytes** en la memoria ARM huésped:
atributos en +0, campo reservado en +4, tamaño real en +8,
nombre de archivo terminado en NUL dentro de los 64 bytes en +12.
El nombre es la ruta relativa normalizada del módulo y no se interpreta
como una ruta absoluta de PS Vita. El otro método
`IFile::GetInfoEx` sigue usando su estructura **diferente de 16 bytes**
con tamaño en +12: no se mezclan formatos.

La consulta respeta el directorio privado del juego, rechaza rutas
absolutas y `..`, comprueba que los 76 bytes de destino sean válidos
**antes** de escribir y devuelve fallo si el archivo no existe.
No se realiza ninguna escritura a datos originales.

## Pruebas

`tests/test_native_zeebx_file_info.c` crea un archivo de prueba
temporal sin ROM. Comprueba el tamaño, el nombre, el formato binario,
celdas nulas, llamadas sin estructura, consultas antes y después de
abrir el recurso, cursor intacto, errores de memoria, protección de
directorio, devolución por archivo inexistente y cierre.

El pipeline portátil incluye ahora **48 pruebas sin ROM**. El
enlace VitaSDK está separado. Ningún ensayo sintético prueba que
`cnk2.mod` pueda renderizar o jugar una carrera completa en Vita.

## Qué falta para «instalar y jugar»

La ruta de archivos originales dentro de `data.vfs` todavía no tiene
un traductor general de nombres internos a recursos FUFS verificables.
Faltan otras llamadas BREW, salida sonora original, programación
continua por cuadro, más gráficos y validación física de un circuito
completo. No existe VPK final y no se empaquetan datos comerciales en
este repositorio.
