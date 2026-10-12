# Avance 147: lectura de archivos originales mediante Zeebx

Se adaptó la ABI de `IFileMgr` (interfaz 3) e `IFile` (interfaz 4) de
`ZeebxTeam/zeebx-emu/src/machine/file.rs` y `src/brew/aee_slots.rs`.
Referencia fija `f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9`,
licencia GPL-2.0-or-later.

El juego invitado puede abrir archivos de su directorio privado, leer
sus bytes reales, buscar posiciones, comprobar existencia, obtener
tamaño mediante `GetInfoEx` (16 bytes, longitud en +12), y cerrar
el `IFile` cuando su recuento de referencias llega a cero. Los
punteros y las tablas virtuales son objetos reales de la memoria
virtual BREW. El probe configura automáticamente como raíz el
directorio del módulo original `cnk2.mod`, compatible con
instalaciones privadas en `app0:/` o datos externos del usuario.

Límites: 8 archivos simultáneos, 1 MiB por lectura. Prohibidas
rutas de escape `../`, rutas absolutas y escrituras. Los errores de
memoria o métodos no portados interrumpen la llamada sin simular
éxito. El reporte del probe registra lecturas, bytes entregados y
rutas rechazadas.

**CI: 46 de 46 pruebas sin ROM aprobadas**:
https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38279908425

**Todavía falta** descomprimir/asociar las entradas del archivo
original `data.vfs` con las rutas que espera el programa, completar
la secuencia del applet y audio/gráficos y verificar en una PS Vita
física. La lectura de archivos sueltos no demuestra que una carrera
original ya funcione. No se publicó ROM, MOD, FUFS ni VPK del juego.
