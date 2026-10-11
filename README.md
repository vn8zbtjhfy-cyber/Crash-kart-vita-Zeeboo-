# Crash Nitro Kart 3D para PS Vita (desarrollo)

Repositorio **privado** para verificar el código del port nativo en PS Vita.

## Estado

Este repositorio aún **no contiene todo el código fuente**. Se ha preparado la configuración de GitHub Actions para ejecutarse cuando se suba el paquete de código limpio del avance 101.

- La comprobación automatizada compila el código C en Linux y ejecuta ocho pruebas que no necesitan la ROM.
- Las pruebas con `data.vfs`, la compilación VitaSDK y la ejecución en una PS Vita real siguen pendientes.
- **No subir** `data.vfs`, `cnk2.mod`, `cnk2.sig`, música, texturas originales ni VPK.
- El proyecto no es todavía un juego terminado.

Para publicar el código completo, extraer el paquete `CrashNitroKart3D_GitHub_CI_Avance101_Limpio.zip` y subir su contenido a este repositorio, respetando `.gitignore`.

Los indicadores verdes en Actions demuestran únicamente las comprobaciones ejecutadas, no que el videojuego funcione en PS Vita.
