# PS Vita: primera prueba física del avance 125

**Paquete experimental. No es el juego terminado.** Lo solicitó el propietario
para descubrir fallos reales de hardware; no reemplaza el trabajo nativo previo
ni distribuye ROM, `cnk2.mod`, `274214.mif`, `data.vfs` o imágenes originales.
El empaquetado está deshabilitado por defecto y solo puede activarse con
`CNK_ENABLE_VITA_DIAGNOSTIC_VPK=ON` en un build VitaSDK.

## Descargar el instalador cuando CI termine en verde

En el repositorio privado, abre el workflow
**CNK3D Vita hardware test VPK**, entra en la ejecución aprobada y
en **Artifacts** descarga **CNK3D-Vita-Hardware-Test-VPK**.
Descomprime el ZIP; dentro debe aparecer
`CNK3D-HARDWARE-TEST.vpk`. No descargues el ELF: el ELF no se instala.

## Preparar la PS Vita

1. Usa una PS Vita con homebrew y VitaShell ya funcional.
2. Copia el VPK a `ux0:data/` vía USB o FTP de VitaShell,
   luego selecciónalo y pulsa X para instalar la burbuja **CNK3D TEST**.
   Evita borrar o sustituir una instalación distinta.
3. Desde tu copia original legal del juego, coloca **fuera del VPK**
   `data.vfs` en `ux0:data/cnk3d/data.vfs` (crea la carpeta si falta).
4. Opcional, para probar también el arranque original con Zeebx: coloca
   `cnk2.mod` y `274214.mif` en la misma carpeta y crea allí el
   archivo vacío `zeebx_probe.flag`. Este sondeo puede consumir memoria
   y causar lentitud. Si solo quieres probar el motor nativo, NO pongas
   el archivo flag.
5. Inicia **CNK3D TEST**. Si aparece el selector o la pista, intenta usar
   cruceta o stick y prueba una carrera. Pulsa **START + SELECT** para salir
   (si los controles ya están inicializados).

## Qué enviar después de la prueba

- ¿Se instaló la burbuja y abrió la app?
- ¿Qué aparece exactamente: pantalla negra, menú, pista, error o cierre?
- Si los ves en VitaShell, comparte como texto o adjunto:
  `ux0:data/cnk3d_last_status.txt` y
  `ux0:data/cnk3d_perf.txt`. El primero contiene código y etapa.
- Si activaste el sondeo Zeebx, comparte
  `ux0:data/cnk3d/zeebx_probe.txt`, que indica la primera API BREW o
  instrucción ARM sin soporte.

Un cierre que ocurra antes de escribir los archivos es información útil;
no lo confundiremos con una prueba aprobada. No compartas públicamente
archivos del juego comercial. Este VPK **no certifica gráficos originales,
jugabilidad, estabilidad ni 60 FPS**.

## Compilar manualmente con VitaSDK

```sh
cmake -S . -B build-vita-hardware \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCNK_ENABLE_VITA_ELF_LINK_CHECK=ON \
  -DCNK_ENABLE_VITA_DIAGNOSTIC_VPK=ON
cmake --build build-vita-hardware --parallel 4
```

El artefacto esperado es `build-vita-hardware/CNK3D-HARDWARE-TEST.vpk`,
sujeto a que el build VitaSDK termine correctamente.
