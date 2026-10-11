# Crash Nitro Kart 3D para PS Vita (desarrollo)

Port nativo en C aún inacabado. No hay ELF ni VPK funcional certificado.

## Estado de avance 107

- 129/129 pruebas PC/Linux en CTest y 2/2 pruebas focalizadas ASan/UBSan.
- Mejorada salida de errores Vita, registro en ux0:data/cnk3d_last_status.txt.
- En GitHub privado existe el flujo VitaSDK, pero antes de publicar este ZIP de fuentes limpias no está el código completo.
- Imagen oficial de VitaSDK: vitasdk/vitasdk:2026.08.

Para publicar solo código, extrae este ZIP en la raíz del repositorio, o usa scripts/publicar_fuentes_git.ps1 desde Windows con git instalado y autenticado. Nunca publiques el ZIP completo de recursos originales.

La acción de GitHub debe enlazar un ELF de diagnóstico. Esto no demuestra funcionamiento en consola y NO genera VPK.
