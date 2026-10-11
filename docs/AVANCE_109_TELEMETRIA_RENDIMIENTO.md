# Avance 109: telemetría mínima para futuras pruebas en PS Vita

Fecha: 11 de octubre de 2026.

## Implementación

- Nuevo módulo C99 independiente de hardware `cnk_vita_telemetry`.
- Tras cada vblank completado, toma una marca de tiempo de Vita y cuenta un cuadro presentado.
- Registra intervalos con tiempo real transcurrido, máximo, promedio en microsegundos y cantidad de cuadros con intervalo >33,333 microsegundos.
- Los intervalos mayores que cinco segundos se cuentan como posibles suspensiones y **no** inflan el promedio de cuadros. También se detectan retrocesos en el reloj.
- En salidas normales o por fallos, se intenta escribir `ux0:data/cnk3d_perf.txt`; la imposibilidad de guardar la telemetría nunca sustituye al error principal.
- La telemetría no recoge información personal ni lee contenido de la ROM ni partidas guardadas. No se llama FPS 60 garantizados ni mide performance real sin una PS Vita física.

## Validación verificable

- Suite de PC con data.vfs original privada: **130/130** pruebas Release completadas.
- Tres pruebas focalizadas con ASan/UBSan: `native_vita_loop`, `vita_diagnostics`, `vita_telemetry` aprobadas.
- Pendiente la compilación ARM de esta revisión concreta y cualquier prueba física en consola.

## Limitación

El primer ELF ARM ya se compiló y auditó en el avance 108, pero **no existe VPK final ni juego terminado**. Conservar porcentaje histórico estimado 45.9 % hasta pruebas hardware y jugabilidad integral. No confundir pruebas de PC con consola.
