# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** del port nativo de Zeebo a PlayStation Vita. **No es todavía un juego completo ni hay un VPK verificado.**

## Avance 108 (11 de octubre de 2026)

- **Primer enlace de ejecutable ELF para ARM/PS Vita aprobado:** GitHub Actions [38185478090](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38185478090), commit `4e2d24e`.
- **Auditoría ARM y archivo ELF de diagnóstico aprobados:** GitHub Actions [38185657768](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38185657768), commit `8c18777`. Se verificaron `ELF32`, `ARM`, `main` y `cnk_vita_native_run`. El ejecutable no es instalable; el artefacto expira según la política de retención.
- **129 de 129 pruebas de PC locales pasaron**, incluida la prueba del bucle nativo simulado y liberación de texturas. La validación automática de GitHub **sin ROM** también pasó con sus ocho pruebas seleccionadas; no equivale a probar una Vita real.
- Corregidas dependencias de enlace: `vitaGL`, `stdc++`, `mathneon`, `taihen_stub` y stubs del sistema, además de la llamada obsoleta `vglEnd()`.

## Qué todavía falta

La compilación ARM **no certifica** que arranque el juego en la consola. Siguen pendientes las pruebas en PS Vita física, compatibilidad de gráficos y audio, memoria real, reproducción de todos los recursos y mecánicas originales, optimización y verificación final de jugabilidad. El porcentaje global estimado se conserva en **45.9 %** hasta obtener pruebas en consola.

## Integridad y derechos de datos

Este repositorio contiene **solamente código, herramientas, pruebas y configuración**. No incorporar a GitHub `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROM, música, modelos ni texturas originales. El paquete privado con materiales proporcionados por el usuario se conserva **separado** del código limpio.

## Cómo comprobarlo

Los workflows `.github/workflows/verificacion.yml` y `.github/workflows/vita-elf-link.yml` verifican código en Linux y enlazan el ELF mediante VitaSDK. El segundo ejecuta `scripts/vita_elf_preflight.sh`, audita la arquitectura y archiva el ELF de diagnóstico. **No se genera SELF ni VPK.**
