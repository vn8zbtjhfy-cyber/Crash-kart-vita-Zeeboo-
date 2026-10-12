# Avance 145: despachar el arranque real IApplet de Crash con Zeebx

## Código implementado

La implementación anterior podía terminar `AEEMod_Load` y llamar a
`IModule::CreateInstance` del **módulo original**, pero no disparaba el
siguiente paso del ciclo de vida BREW. El emulador Zeebx usa
`Machine::start_applet` en `src/machine/shell.rs`:

1. Lee desde la memoria ARM del juego el puntero de `IApplet` y su
   `HandleEvent` en el **slot 2 de la tabla virtual**.
2. Crea un objeto `IDisplay` invitado y una estructura `AEEAppStart` de
   24 bytes (código de error, ClassID, `IDisplay*`, rectángulo 640×480,
   argumentos nulos).
3. Llama al manejador ARM original con cuatro registros AAPCS:
   `r0=IApplet*`, `r1=EVT_APP_START=0`, `r2=0`,
   `r3=AEEAppStart*`.

`cnk_zeebx_arm_prepare_start_event` adapta esos pasos directamente al
submotor ARM/BREW del proyecto. El sondeo opcional ejecuta después el
código original del `HandleEvent` con **presupuesto acotado de
instrucciones**, sin fingir éxito de APIs que aún faltan.

## Evidencias, limitaciones y seguridad

- La prueba sin ROM construye un módulo ARM sintético que crea su propia
  instancia y responde al evento real. Comprueba sus registros, memoria
  `AEEAppStart`, rectángulo, ClassID, punteros y retorno.
- Una entrada de vtable inválida se rechaza **antes** de consumir heap,
  generar `IDisplay` o modificar el PC; las entradas Thumb todavía
  no se consideran compatibles.
- El diagnóstico agrega `start_event_attempted`,
  `start_event_prepare_code`, `start_event_entry`,
  `start_event_data`, `start_event_reason`,
  `start_event_api_interface`, `start_event_api_slot` y
  `start_event_completed`.
- Un retorno del handler **NO demuestra** que exista menú o carrera
  jugable. Falta procesar servicios BREW solicitados durante
  `EVT_APP_START`, timers, acceso al sistema original de archivos y
  reproducción de audio y gráficos restantes, más QA en PS Vita física.
- Los archivos comerciales (`cnk2.mod`, `data.vfs`, `274214.mif`)
  NO se incorporaron al repositorio ni a GitHub Actions.

Referencia: ZeebxTeam/zeebx-emu, revisión
`f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9`,
GPL-2.0-or-later; implementación adaptada, no emulación genérica
completa para PS Vita. Todavía no se entrega un VPK final.
