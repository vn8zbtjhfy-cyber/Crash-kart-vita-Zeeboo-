# Avance 116: control opcional por inclinación de PS Vita a partir de investigación de Zeebx

## Evidencia externa (NO código incorporado)
El proyecto ZeebxTeam/zeebx-emu (rama development; GPL-2.0-or-later) publica
[una investigación del Boomerang](https://github.com/ZeebxTeam/zeebx-emu/blob/development/docs/implementacao/20-boomerang-e-wii-remote.md)
que documenta el uso de acelerómetro en Crash Nitro Kart 3D y su calibración.
Su [registro de Crash](https://github.com/ZeebxTeam/zeebx-emu/blob/development/docs/varredura/crash-bandicoot-nitro-kart-3d--brazil---es-pt.txt)
también identifica rutas de archivos solicitados (ejemplos: `data/levels/adven/1.trk`
y `data/carts/chars/crash.pof`). Estas listas **no establecen** un índice FUFS ni
una correspondencia verificable entre archivos, personajes y modelos del port.

El código de este avance es original, en C, sin copiar fuentes de Zeebx, sin
incorporar la biblioteca de emulación ni modificar la licencia del repositorio.

## Lo implementado
- `cnk_motion_steer.[ch]` convierte variaciones laterales respecto al valor de
  calibración en dirección [-1,1], con banda muerta, suavizado y rechazo de
  muestras inválidas. Coeficientes provisionales, NO físicos originales Zeebo.
- `cnk_vita_motion.[ch]` usa `sceMotionStartSampling`,
  `sceMotionGetSensorState`, `sceMotionStopSampling` (VitaSDK,
  [SceMotion](https://docs.vitasdk.org/group__SceMotionUser.html)).
- **SELECT durante una carrera** activa/desactiva la inclinación; SELECT sigue
  cambiando LANG en el selector de pistas y START+SELECT sigue saliendo.
- El stick o la cruceta tienen prioridad. En un fallo del sensor no se
  cancela la carrera; el control convencional continúa disponible.
- Al regresar al selector o cerrar la aplicación se desactiva el muestreo.
- Se ve `SELECT TILT ON/OFF` en la interfaz provisional de carrera.

## Alcance de la comprobación
Las pruebas host `motion_steer` y `vita_motion` simulan lecturas de sensor,
ruido, saturación, errores de inicio, caída transitoria y cierre. El test
`native_vita_loop` enlaza el adaptador con un sensor simulado. El CI sin ROM
sigue auditando que no se incluya `data.vfs`, ROM, música, texturas o VPK.

**Esto NO confirma ni la orientación de ejes ni la sensación de manejo en una
PS Vita física.** La inclinación requiere calibración y validación con hardware.
Tampoco recupera el código de conducción BREW original.

No se crea SELF ni VPK. No se suben archivos de recursos originales.
