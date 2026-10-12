# Avance 123: servicios reales del ejecutable Zeebo en el intérprete Vita

## Alcance

Sin reemplazar el motor original del port a PS Vita, extendimos la ruta
optativa de CPU ARM derivada del emulador Zeebx (GPL-2.0-or-later).
Las funcionalidades C99 nuevas forman parte del núcleo del juego, no son
meras notas. El intérprete **no ejecuta todavía el juego completo**.

## APIs trasladadas con fundamento en Zeebx

- **IShell::CreateInstance**, slot 2. Interpreta los ClassID originales
  identificados por Zeebx: `0x01001001` → IDisplay (interfaz 5),
  `0x01001003` → IFileMgr (interfaz 3) y
  `0x01001056` → ISound (interfaz 14).
  Crea objetos reales *en memoria virtual guest*, con puntero a sus vtables
  de 256 slots y direcciones de trampolín para cada método. Las llamadas
  a esos métodos todavía no implementados se detienen con diagnóstico
  en vez de fingir una imagen o audio.
- **IShell::GetDeviceInfo**, slot 4. Escribe en memoria virtual los campos
  BREW conocidos del dispositivo Zeebo: pantalla 640×480, profundidad 16
  bits, codificación ISO Latin-1 y estructura extendida de 64 bytes
  si fue solicitada. Verifica rango del buffer antes de escribir.
  La cifra de 64 MiB corresponde al dispositivo virtual de Zeebx,
  **no** a la memoria física real de PS Vita.
- **AEEStdLib::memset**, helper 1. Modifica bytes del guest, respeta
  límites, devuelve el puntero como en Zeebx.
- **dbgprintf**, helper 39. Registra un formato de cadena del guest
  acotado. No lo pasa como cadena de formato al `printf` del host.
- **aee_GetTimeMS** y **aee_GetUpTimeMS**, helpers 43/44, comparten el
  reloj *virtual* de Zeebx que inicializa en 0 y que puede avanzar el
  controlador. No se adivina tiempo a partir del número de opcodes.
- Se conserva `malloc`, `IShell::AddRef/Release`, tablas BREW,
  verificación del ClassID desde el MIF original y la ruta de
  `IModule::CreateInstance` del avance 122.

Fuentes de referencia de Zeebx:
- `src/machine/shell.rs` (factory y GetDeviceInfo),
- `src/machine/helper.rs` y `src/brew/aee_helpers.rs` (stdlib),
- `src/brew/aee.rs` (índices de interfaz),
- `src/loader/mod.rs` (regiones guest).
Código adaptado: `src/cnk_zeebx_arm.[ch]`. Se aplican las
obligaciones de licencia GPL-2.0-or-later.

## Prueba en el archivo de Crash que el usuario ya proporcionó

En un entorno **local y privado** de referencia Python, se interpretó
`cnk2.mod` auténtico (nunca subido al repositorio):

1. `AEEMod_Load` retornó correctamente tras 21,185 instrucciones ARM,
   con `IModule*` real.
2. La etapa `IModule::CreateInstance` creó los objetos `IDisplay`,
   `IFileMgr` e `ISound`, solicitando GetDeviceInfo, varias operaciones
   `memset` y `aee_GetUpTimeMS`.
3. Implementados esos servicios en la referencia local, la etapa alcanzó
   **IDisplay::GetDeviceBitmap**, interfaz 5, slot 16, luego
   **IBitmap::QueryInterface**, interfaz 7, slot 2, después de aprox.
   6,414 instrucciones **de la segunda etapa**.

Esa última API de Bitmap todavía está pendiente de trasladar a C; el
reporte significa **localización precisa del siguiente trabajo**,
no gráficos que se renderizan en la consola. La prueba local Python no
sustituye una prueba real de ejecución del mismo código C y mucho menos
una prueba en la PS Vita física.

## Pruebas automáticas reproducibles

Las pruebas de GitHub usan binarios **sintéticos** que contienen auténticas
instrucciones ARM A32 y llamadas BREW; comprueban objetos y vtables,
salida de clases, estructura DeviceInfo, reloj virtual, escrituras memset
y que servicios o punteros desconocidos se detengan de forma segura.
Todos los tests previos del port se conservan.

No se incluye ROM, módulo original `.mod`, `data.vfs`, sonido, texturas,
SELF ni VPK en este avance. La ruta de emulador permanece desactivada salvo
que se habilite explícitamente el diagnóstico; el motor nativo de carreras
no se ha reemplazado. El porcentaje global histórico no debe aumentar
por el solo hecho de ejecutar más opcodes hasta verificar jugabilidad.
