# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** del port nativo de Zeebo a PlayStation Vita. **Todavía no es un videojuego completo ni se ha verificado su ejecución en consola.**

## Avance 112 (11 de octubre de 2026)

- **132/132 pruebas de PC Release aprobadas** con los materiales originales del usuario conservados fuera del repositorio; test del selector externo de FUFS aprobado también con ASan/UBSan.
- **13 pruebas de PC sin ROM aprobadas en GitHub Actions**: [ejecución 38186987038](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38186987038).
- **VitaSDK compiló y enlazó el ELF ARM** con la nueva detección de archivo `data.vfs`: [ejecución 38186987085](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38186987085), commit `b2c2065`.
- El punto de entrada `src/vita_main.c` ya busca el `data.vfs` original del usuario **fuera del futuro VPK**. Ubicación preferida en la consola: `ux0:data/cnk3d/data.vfs`, seguida de `ux0:data/data.vfs` y la ruta heredada `app0:/data.vfs`. Se descartan archivos sin cabecera `FUFS`; después, el motor verifica el archivo completamente al abrirlo.
- Se conservan los avances de telemetría, control y protección de geometría. Ver `docs/AVANCE_111_RUTA_EXTERNA_FUFS.md` y `docs/AVANCE_111_GRILLA_GEOMETRIA_FINITA.md`.
- **Un ELF compilado no es un VPK funcional ni demuestra que una PS Vita real pueda jugarlo.**

## Qué todavía falta

La compilación ARM **no certifica** que arranque el juego en la consola. Siguen pendientes las pruebas en PS Vita física, compatibilidad de gráficos y audio, memoria real, reproducción de todos los recursos y mecánicas originales, optimización y verificación final de jugabilidad. El porcentaje global estimado se conserva en **45.9 %** hasta obtener pruebas en consola.

## Integridad y derechos de datos

Este repositorio contiene **solamente código, herramientas, pruebas y configuración**. No incorporar a GitHub `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROM, música, modelos ni texturas originales. El paquete privado con materiales proporcionados por el usuario se conserva **separado** del código limpio.

## Cómo comprobarlo

Los workflows `.github/workflows/verificacion.yml` y `.github/workflows/vita-elf-link.yml` verifican código en Linux y enlazan el ELF mediante VitaSDK. El segundo ejecuta `scripts/vita_elf_preflight.sh`, audita la arquitectura y archiva el ELF de diagnóstico. **No se genera SELF ni VPK.**
