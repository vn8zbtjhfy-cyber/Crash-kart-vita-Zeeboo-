# Avance 146: temporizadores BREW originales desde Zeebx

Se adaptaron al núcleo C99 los métodos reales de
`ZeebxTeam/zeebx-emu/src/machine/shell.rs`:

- **IShell::SetTimer** (slot 11): el plazo es `int32`; los negativos
  se interpretan como vencidos ya. La misma pareja de
  `PFNNOTIFY` y contexto rearma/reemplaza el temporizador anterior.
- **CancelTimer** (slot 12): cancela una pareja exacta o todos los
  temporizadores del contexto cuando el callback es nulo.
- **GetTimerExpiration** (slot 13): devuelve milisegundos restantes
  del reloj virtual, nunca el tiempo de la computadora. El callback
  nulo devuelve el `EBADPARM` real, valor 2.
- **cnk_zeebx_arm_deliver_timers**: invoca las funciones ARM originales
  exclusivamente cuando su reloj virtual alcanzó el plazo, desde una
  frontera segura de retorno del applet. Guarda y restaura los
  registros y CPSR, limita instrucciones, y nunca ejecuta punteros
  de la ROM como código nativo de Vita.
- Se conservan hasta 32 temporizadores por sesión invitada.
  Cuando no cabe otro, la API se detiene con diagnóstico en vez de
  sobreescribir memoria. También se limita el número de callbacks
  continuos con retardo cero.
- El diagnóstico opcional de inicio intenta entregar los
  temporizadores **ya vencidos** después del retorno del
  `EVT_APP_START`. No adelanta artificialmente el reloj 16 ms ni
  finge que existe todavía un bucle original por cuadros.

Las pruebas de CI usan código ARM sintético, prueban vencimiento,
consulta, reprogramación, cancelación, valores negativos, cola llena,
puntero invitado inválido y preservación de registros. **No prueba
el juego original en hardware**.

Sigue pendiente conectar el reloj virtual y la entrada física de Vita
a un bucle invitado persistente, además de los servicios de archivos,
audio, GLES y recursos que falten. **No equivale a una carrera jugable**.

El código reutilizado se atribuye a ZeebxTeam y su licencia
GPL-2.0-or-later; no se distribuyen ROM, MOD, MIF, VFS ni VPK final.
