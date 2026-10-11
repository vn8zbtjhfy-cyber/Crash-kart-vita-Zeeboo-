# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** del port nativo en C. **No es un juego terminado, no existe VPK verificado y todavía no hay un ELF enlazado con VitaSDK.**

## Estado: avance 107 local, subida parcial

- En el entorno de desarrollo se verificaron **128/128 pruebas de PC** y dos pruebas focalizadas con ASan/UBSan.
- Ya están publicados en este repositorio el flujo **VitaSDK diagnostic ELF**, el script `scripts/vita_elf_preflight.sh` y `src/cnk_vita_diagnostics.[ch]`.
- **El repositorio aún no contiene la totalidad de las fuentes** (por ejemplo, falta `CMakeLists.txt` y el resto de `src/`). La acción de VitaSDK no puede producir un ELF hasta que el paquete completo de **fuentes limpias** se publique.
- El contenedor de CI usa `vitasdk/vitasdk:2026.08` e intenta compilar y enlazar un ELF de diagnóstico. **No genera SELF ni VPK**.

## Fuente limpia y respeto a los datos originales

Publicar solamente el contenido del archivo `Crash_Nitro_Kart_Vita_Avance107_GitHub_Solo_Codigo.zip` entregado en la conversación. Extraer su contenido dentro de la raíz del repositorio, sin añadir otra carpeta envolvente.

**Nunca subir** `data.vfs`, `cnk2.mod`, `cnk2.sig`, ROM, música, modelos/texturas originales ni claves privadas. El ZIP completo del juego se conserva separado y **no** se debe subir.

Tras publicar todo el código en la rama `main`, GitHub Actions realizará la prueba de compilación ARM. Un indicador verde certificará solo el proceso ejecutado, **no** que el juego funcione en PS Vita. La prueba en consola física, rendimiento, fidelidad a Zeebo y recuperación de funcionalidades siguen pendientes.
