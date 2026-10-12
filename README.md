# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** del port nativo de Zeebo a PlayStation Vita. **Todavía no es un videojuego completo ni se ha verificado su ejecución en consola.**

## Avance 110 (11 de octubre de 2026)

- **130/130 pruebas de PC Release** con los recursos privados y **2/2 pruebas seleccionadas ASan/UBSan** para el control del mando y el bucle de Vita.
- **11/11 pruebas de PC sin ROM aprobadas en GitHub Actions**, incluyendo telemetría y diagnóstico: [ejecución 38186433951](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38186433951).
- **VitaSDK aprobó la compilación y enlace ARM ELF** para la versión del avance 110: [ejecución 38186433962](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38186433962), commit `bdc6825`.
- Avance 109: recopilación mínima de rendimiento en `ux0:data/cnk3d_perf.txt` para que, **cuando pueda ejecutarse en Vita**, podamos medir cuadros, tirones y suspensiones sin confundirlos con fallos de renderizado.
- Avance 110: la lectura del mando tolera hasta siete muestras vacías seguidas con **entrada neutral**, recupera tras la siguiente lectura válida y rechaza fallos persistentes o errores reales.
- Ver `docs/AVANCE_108_ENLACE_ARM_ELF.md`, `docs/AVANCE_109_TELEMETRIA_RENDIMIENTO.md` y `docs/AVANCE_110_CONTROLES_MUESTRAS_VITA.md`.

## Qué todavía falta

La compilación ARM **no certifica** que arranque el juego en la consola. Siguen pendientes las pruebas en PS Vita física, compatibilidad de gráficos y audio, memoria real, reproducción de todos los recursos y mecánicas originales, optimización y verificación final de jugabilidad. El porcentaje global estimado se conserva en **45.9 %** hasta obtener pruebas en consola.

## Integridad y derechos de datos

Este repositorio contiene **solamente código, herramientas, pruebas y configuración**. No incorporar a GitHub `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROM, música, modelos ni texturas originales. El paquete privado con materiales proporcionados por el usuario se conserva **separado** del código limpio.

## Cómo comprobarlo

Los workflows `.github/workflows/verificacion.yml` y `.github/workflows/vita-elf-link.yml` verifican código en Linux y enlazan el ELF mediante VitaSDK. El segundo ejecuta `scripts/vita_elf_preflight.sh`, audita la arquitectura y archiva el ELF de diagnóstico. **No se genera SELF ni VPK.**
