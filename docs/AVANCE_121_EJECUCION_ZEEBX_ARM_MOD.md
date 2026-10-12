# Avance 121: código ARM original, ejecución BREW y fuente completa de Zeebx

## Qué se recuperó y qué NO se destruyó

El port C existente de Crash Nitro Kart 3D para Vita sigue intacto:
renderizador vitaGL, circuitos FUFS/SCEN, simulación de vehículos,
objetos, sonidos, UI original y controles. **El módulo de ejecución
de Zeebx se agregó como una ruta complementaria, aislada y
DESACTIVADA por defecto.** No introduce un emulador genérico en
el bucle normal del juego.

El **repositorio completo** de Zeebx (licencia GPL-2.0-or-later)
queda vinculado como un submódulo Git en `third_party/zeebx_emulator`,
fijado en `f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9`.
Para consultar TODO su código:
```bash
git submodule update --init --recursive
```
Esto no lo compila automáticamente para Vita: Zeebx original es un
proyecto Rust con dependencias y rutas de escritorio/WebAssembly, y
contiene interfaces que **todavía necesitan adaptarse** a VitaSDK.
El submódulo conserva toda la referencia sin rehacer el motor nativo.

## Código que SÍ adaptamos y ejecutamos en el port

- `src/cnk_zeebx_arm.[ch]`: subset C99 del intérprete ARM1176 de
  Zeebx (`src/cpu/interpretador.rs`): fetch de opcodes reales de
  invitado, registros, CPSR, condición, ALU, B/BL, BX/BLX, memoria,
  carga/almacenamiento, bloques de registros y trampolines BREW.
- Memoria virtual compatible con el `loader/mod.rs` de Zeebx:
  módulo en `0x00010000`, prefix helper, pila, objetos, vtables,
  tabla de helpers y heap de arranque.
- Helpers de arranque **no simulados por éxito genérico**:
  `AEEStdLib::malloc` (interfaz 6, slot 26) con memoria guest
  real reservada y `IShell::AddRef` (interfaz 0, slot 0).
  Una llamada BREW desconocida DETIENE el intérprete y queda reportada.
- `src/cnk_zeebx_probe.[ch]`: carga un `cnk2.mod` original que
  el usuario guarda localmente, ejecuta hasta el primer límite
  comprobable y genera una traza sin exportar el binario.
- `src/vita_main.c`: prueba **opt-in** con un archivo marcador.
  El programa continúa después con el juego C nativo de siempre.
  No cambia física, pistas ni el flujo de carrera.

## Evidencia con el cnk2.mod ORIGINAL del usuario

Una prueba **local de referencia en PC** interpretó el ejecutable
`cnk2.mod` original (649452 bytes, sin incluirlo en este Git):

- Primera detención BREW: instrucción guest **21154**,
  `AEEStdLib::malloc` (6,26), reservando 36 bytes.
- Segunda llamada BREW: instrucción guest **21178**,
  `IShell::AddRef` (0,0).
- `AEEMod_Load` finalizó tras **21185 instrucciones**,
  con retorno R0 = 0 y `IModule*` generado en `0x10000000`
  (heap guest real con 40 bytes alineados).
- La prueba local usó un intérprete Python de referencia que
  traduce el mismo subset de Zeebx. El ejecutable C integrado
  posee pruebas sintéticas para esos dos helpers. **Estos números
  NO se presentan como un resultado de Vita física ni como test de
  la versión C contra el archivo real.**

## Cómo comprobarlo en Vita sin instalar nada nuevo

La opción de traza está implementada en `vita_main.c` para cuando
se compile la aplicación nativa en una futura versión verificable.
Debes tener tus archivos **privados**:
- `ux0:data/cnk3d/data.vfs` (ya usado por el port).
- `ux0:data/cnk3d/cnk2.mod` (nuevo, solo en tu consola).
- `ux0:data/cnk3d/zeebx_probe.flag` (archivo vacío activa la traza).

Cuando exista una versión instalable probada, el proceso escribirá
`ux0:data/cnk3d/zeebx_probe.txt` con:
número de instrucciones guest, PC, opcode no soportado, primera API
no implementada, puntero IModule y resultado de `AEEMod_Load`.
Si NO hay marcador, **no usa el intérprete** y corre el port normal.
No se genera SELF ni VPK en esta fase.

## Pruebas, próximos obstáculos y límites

- Las pruebas `zeebx_arm`, `zeebx_probe` y `zeebx_bootstrap` usan
  instrucciones **sintéticas**. CI valida CPU, Saltos, flags,
  pila, límites de ejecución, llamada helper, creación de módulo
  guest, y freno ante API desconocida. El resto de las pruebas del
  port siguen intactas.
- Al pasar `AEEMod_Load` todavía falta
  `IModule::CreateInstance`, eventos de applet, llamadas BREW
  de sistema/archivos, EGL/GLES, mando y audio. El intérprete C
  todavía **NO incluye Thumb completo ni Dynarmic**.
- CPU de invitado y memoria virtual son solo una parte de Zeebx;
  **cargar el árbol completo de código** no equivale a tener un
  emulador completo funcional compilado para Vita.
- Una carrera jugable y rendimiento en PS Vita física aún no están
  demostrados, y no procede elevar por ello el porcentaje global.

### Licencias y datos privados

Zeebx es GPL-2.0-or-later. Las adaptaciones conservan
`third_party/zeebx/LICENSE` y `NOTICE.md`. Una distribución
final que vincule este código debe cumplir las obligaciones de
fuente/licencia GPL aplicables.

No se sube ningún `cnk2.mod`, `274214.mif`, `data.vfs`,
ROM, textura o música. No se produce SELF ni VPK.
