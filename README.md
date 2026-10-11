# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** de código fuente para el port nativo de Zeebo a PS Vita. **Todavía no es un juego completo ni existe un VPK verificado.**

## Estado: avance 107 publicado (11 de octubre de 2026)

- Publicados **171 archivos del paquete limpio**, más **142 archivos de pruebas y herramientas de compilación**, manteniendo el `.gitignore` protector que ya existía.
- **129/129 pruebas de PC** aprobadas en la sesión de desarrollo con los recursos locales. La validación automatizada sin ROM de GitHub Actions para el commit `9463b8b` **pasó** con las ocho pruebas portátiles configuradas. Esto no demuestra ejecución en la consola.
- Los workflows `Validación de código (sin ROM)` y `VitaSDK diagnostic ELF` se ejecutan después de publicar en `main`.
- Corregida una llamada a `vglEnd()` que no existe en la versión actual de la API pública de vitaGL. La compilación con VitaSDK y el enlace ARM se comprueban con GitHub Actions. El arranque en PS Vita, la jugabilidad y el rendimiento siguen pendientes y deben comprobarse **por separado**. Un ELF compilado no equivaldría todavía a un juego terminado.

## Fuentes y recursos

**Nunca publicar** `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROMs, archivos musicales o texturas/modelos originales en este repositorio. El ZIP completo del proyecto se mantiene separado para las pruebas privadas.

## Diagnóstico de compilación

El archivo `.github/workflows/vita-elf-link.yml` intenta ejecutar `scripts/vita_elf_preflight.sh` con el contenedor VitaSDK. **Ese flujo NO produce SELF ni VPK**. Los errores y logs de GitHub Actions son el punto de partida para corregir la compilación.
