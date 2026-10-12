# Avance 123: primeras clases BREW solicitadas por Crash Nitro Kart real

## Objetivo

Avanzar por el **camino de ejecución exacto del `cnk2.mod` original**
aprovechando el código de Zeebx, sin sustituir el motor de carreras
nativo ni fabricar respuestas exitosas a servicios desconocidos.

## Evidencia obtenida localmente sin publicar la ROM

Se ejecutó la copia privada del `cnk2.mod` del usuario en un
intérprete Python *de referencia* que sigue el mismo subconjunto de
instrucciones ARM del código C adaptado de
[Zeebx ARM1176](https://github.com/ZeebxTeam/zeebx-emu/blob/f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9/src/cpu/interpretador.rs).

- `AEEMod_Load`: retornó tras 21,185 instrucciones ARM, creando
  el `IModule*` real en el heap guest.
- Su vtable ubicó `IModule::CreateInstance` en la dirección virtual
  `0x00010224`. **No se inventó esa dirección.**
- Dentro de `CreateInstance`:
  - primer `IShell::CreateInstance`, ClassID `0x01001001` =
    `AEECLSID_DISPLAY` (Zeebx `Interface::Display = 5`);
  - `IShell::AddRef` y `AEEStdLib::dbgprintf`, este último
    con el texto original `AEEApplet_New: return (TRUE)`;
  - segundo `IShell::CreateInstance`, ClassID `0x01001003` =
    `AEECLSID_FILEMGR` (`Interface::FileMgr = 3`);
  - después de ofrecer esas fábricas, la siguiente detención fue
    **`IShell::GetDeviceInfo`**, interfaz 0, slot 4,
    tras 560 instrucciones de la segunda etapa.
- El resultado anterior es de la **referencia local de PC**.
  **NO** equivale a una carrera, pantalla dibujada o ejecución
  probada del código C en una PS Vita real.

## Integración efectiva al código nativo

En `src/cnk_zeebx_arm.[ch]`:

- El mapa `VTABLE` guest crece de 4 KiB a 64 KiB, para permitir
  el índice de las interfaces de Zeebx de forma auténtica.
- Se inicializan las vtables `IShell`, `IDisplay` e
  `IFileMgr`. Sus slots apuntan a la misma zona de trampa BREW
  identificada por interfaz y número de método.
- `IShell::CreateInstance` **solo** admite las dos ClassID
  confirmadas en Crash: DISPLAY `0x01001001` y FILEMGR
  `0x01001003`. Crea objetos reales en la memoria virtual
  guest con su puntero a la vtable correcta, reserva una
  entrada por objeto y escribe el puntero de salida indicado
  por el propio programa ARM.
- `dbgprintf` (helper 39) captura de forma acotada el texto
  del formato guest y devuelve `AEE_SUCCESS` como Zeebx.
  No interpreta cadenas provenientes del juego como formatos
  del `printf` host, por seguridad.
- Las ClassID desconocidas continúan deteniéndose con un
  resultado `BREW_API`; no se responde éxito por conveniencia.
- El informe privado `zeebx_probe.txt` indica cuántos objetos
  DISPLAY/FILEMGR se crearon y cuántos mensajes de depuración
  solicitó el ejecutable.
- Todo permanece **desactivado por defecto**. El modo diagnóstico
  se activa con `ux0:data/cnk3d/zeebx_probe.flag` junto a
  archivos privados `cnk2.mod` y `274214.mif`. El programa
  sigue ejecutando el port nativo de siempre.

## Fuentes de la implementación

- Zeebx `src/machine/mod.rs`: identifica ClassID DISPLAY/FILEMGR.
- Zeebx `src/machine/shell.rs`: mapea ClassID a
  interfaces y fabrica objetos de la tabla de métodos.
- Zeebx `src/machine/helper.rs`: comportamiento de `dbgprintf`.
- Zeebx `src/brew/aee.rs`: codificación de métodos en
  trampolines de interfaces.

Todos estos archivos forman parte del submódulo completo de
Zeebx fijado a commit `f6d3e7bd...`, GPL-2.0-or-later.
La adaptación C es un trabajo derivado con avisos y fuentes
en `third_party/zeebx`.

## Próxima etapa real

`IShell::GetDeviceInfo` devuelve la información de pantalla,
memoria y dispositivo de Zeebo, con estructuras específicas BREW.
Su implementación deberá basarse en los campos y valores
comprobables de Zeebx, no en responder éxito con una estructura vacía.
Después aparecerán llamadas propias de `IDisplay` y
`IFileMgr`, donde habrá que mapear las entradas y salidas al
renderizador/audio/sistema de archivos de PS Vita.

La prueba `zeebx_brew_factories` utiliza un ejecutable ARM
**sintético sin ROM** y verifica que los objetos aparecen por
escrituras de memoria guest, que las tablas apuntan a métodos
adecuados y que la clase desconocida se rechaza.
No se genera VPK, SELF, ni se sube `data.vfs`.
