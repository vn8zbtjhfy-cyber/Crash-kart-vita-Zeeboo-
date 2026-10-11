# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** de código fuente para el port nativo de Zeebo a PS Vita. **Todavía no es un juego completo ni existe un VPK verificado.**

## Estado: avance 107 publicado (11 de octubre de 2026)

- Publicadas las **171 entradas de fuentes/configuración del paquete limpio**, manteniendo el `.gitignore` protector que ya existía.
- **129/129 pruebas de PC** aprobadas en la sesión de desarrollo. Esas pruebas no demuestran ejecución en la consola.
- Los workflows `Validación de código (sin ROM)` y `VitaSDK diagnostic ELF` se ejecutan después de publicar en `main`.
- La compilación con VitaSDK, el enlace ARM, el arranque en PS Vita, la jugabilidad y el rendimiento deben comprobarse **por separado**. Un ELF compilado no equivaldría todavía a un juego terminado.

## Fuentes y recursos

**Nunca publicar** `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROMs, archivos musicales o texturas/modelos originales en este repositorio. El ZIP completo del proyecto se mantiene separado para las pruebas privadas.

## Diagnóstico de compilación

El archivo `.github/workflows/vita-elf-link.yml` intenta ejecutar `scripts/vita_elf_preflight.sh` con el contenedor VitaSDK. **Ese flujo NO produce SELF ni VPK**. Los errores y logs de GitHub Actions son el punto de partida para corregir la compilación.
