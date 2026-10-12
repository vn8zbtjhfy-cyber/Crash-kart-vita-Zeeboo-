# Avance 124: Bitmap e IDIB originales en memoria virtual de PS Vita

En esta etapa el intérprete ARM C99 basado en Zeebx (GPL-2.0-or-later)
continúa un poco más el arranque del **cnk2.mod original** sin reemplazar
el motor de carreras, circuitos, sonido, controles ni interfaz nativa que
ya están programados.

## Qué se programó

- **IDisplay::GetDeviceBitmap**, implementado desde el avance anterior,
  entrega una superficie persistente con tabla de métodos IBitmap.
- **IBitmap::QueryInterface**, método 2 de Zeebx, ahora reconoce la
  interfaz IBitmap (`0x01001021`) y las interfaces DIB de BREW
  (`0x01001045` actual y `0x0100102c` antigua).
- Al pedir IDIB, reserva **640 × 480 × 2 = 614,400 bytes** de memoria
  *guest* RGB565. El puntero resultante se utiliza de verdad por el
  intérprete para leer/escribir píxeles; no es una respuesta ficticia.
- Publica los campos verdaderos de `AEEIDIB.h`: `pBmp`, `cx=640`,
  `cy=480`, `pitch=1280`, `nDepth=16`,
  `nColorScheme=16` y color clave `0xf81f`.
- Implementa AddRef/Release del bitmap de pantalla respetando la
  referencia del display; las consultas repetidas utilizan **la misma
  superficie**, no crean cientos de objetos.
- Solo se acepta un IBitmap realmente creado por nuestro controlador.
  Clases ajenas siguen siendo una **trampa explícita** con diagnóstico.
- No hay conversión a imagen de la pantalla física Vita todavía:
  **esto es el framebuffer del emulador, no un render completo**.

## Qué dice el ejecutable auténtico, sin publicarlo

La copia original privada de `cnk2.mod` se ejecutó de nuevo con un
intérprete Python de referencia basado en las reglas de Zeebx:
- Tras cargar el módulo y solicitar IDisplay, IFileMgr, ISound,
  DeviceInfo, memset y reloj, llega a
  `IDisplay::GetDeviceBitmap` (5,16), y
  `IBitmap::QueryInterface` (7,2).
- Con esas dos operaciones implementadas en el *modelo privado*,
  alcanza **IShell::CreateInstance** solicitando
  **`0x01014bc4`**, el ClassID original de la interfaz
  **EGL antigua** de Zeebo. Esto sucede aproximadamente en el paso
  6,444 de la función de creación del applet.
- El siguiente trabajo de máxima prioridad será conectar la gestión
  real de EGL/GLES del emulador a VitaGL y al ciclo de fotogramas.

**Diferencia crítica:** esa traza es de un modelo local, no una carrera
jugable, ni prueba del binario compilado en hardware. La prueba CI
`zeebx_idib` sí usa el nuevo intérprete C99 y un guest ARM sintético
para verificar tabla de funciones, datos RGB565, tamaño, memoria,
reutilización y paradas ante interfaces desconocidas.

## Estado

Ningún contenido protegido de `cnk2.mod`, `data.vfs`, PNG o ROM se
almacena en GitHub. El código GPL de Zeebx está atribuido en
`third_party/zeebx/NOTICE.md`, y su fuente completa ya permanece en
el submódulo fijado. No se genera SELF ni VPK antes de terminar y
verificar el juego. Mantener la estimación histórica de progreso hasta
contar con una carrera original ejecutada en Vita.
