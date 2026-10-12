# Crash Nitro Kart 3D para PS Vita (desarrollo)

## Avance 142: color original de texturas GL_REPLACE (Zeebx → vitaGL)

El código original de Crash pide el modo `GL_REPLACE` por medio de
`glTexEnvxv` (modo enviado por **puntero guest**), tal como describe
`ZeebxTeam/zeebx-emu/src/machine/gl.rs`. Se portaron las llamadas
`glTexEnvx` y `glTexEnvxv` del IGL antiguo a C, con validaciones de
memoria y GPU, y se conectaron a `glTexEnvi` de vitaGL. Esto evita
tratar las enumeraciones como punto fijo y permite al juego elegir
cómo combinar los colores de sus texturas al dibujar.

- **44/44 pruebas automáticas sin ROM aprobadas**:
  [GitHub Actions](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38213003070).
- [Informe técnico](docs/AVANCE_142_ZEEBX_TEXTURE_ENV_CRASH.md).
- Sigue **sin prueba de carrera original jugable en PS Vita física**.
  Aún faltan llamadas GL, servicios BREW y el bucle completo del applet;
  no se debe confundir un ELF de diagnóstico con un juego terminado.


## Avance 138: vertex buffers Zeebx para el original

- Soporte original BREW `glGenBuffers`, `glBindBuffer`, `glBufferData`, `glBufferSubData`, `glDeleteBuffers` y `glIsBuffer`, con límites de memoria.
- `glVertexPointer(..., 0)` usa ahora el desplazamiento cero de un VBO enlazado; `glDrawElements` lee los índices del buffer asociado. Se mantienen las conversiones `GL_FIXED`, UV originales, tiras y abanicos y el host de dibujo Vita.
- **41 de 41 pruebas sin ROM aprobadas** en CI. El ELF VitaSDK se ha enlazado, pero **no prueba que el videojuego original sea jugable en la consola**.
- [Documentación técnica](docs/AVANCE_138_ZEEBX_VBO_BUFFERS.md).


## Avance 137: coordenadas GL_FIXED Zeebo para modelos y texturas

- Se extiende el puente original de Zeebx a vitaGL para interpretar `GL_FIXED` 16.16 **con signo** en posiciones 3D y coordenadas UV, además de `GL_FLOAT`; los dos formatos pueden combinarse por separado.
- Se conservan listas indexadas, tiras y abanicos de triángulos, buffers con paso intercalado, y comprobaciones de memoria invitada. Fallos de GPU, tipo no soportado o índice fuera de rango detienen la API original sin fingir que dibujó.
- El bloque incluye una nueva prueba de geometría fija con datos sintéticos, subiendo la selección de CI de 39 a **40 tests sin ROM**. La ejecución final y el enlace VitaSDK se revisan en GitHub Actions.
- [Documentación técnica](docs/AVANCE_137_ZEEBX_GL_FIXED_GEOMETRIA.md).

**Importante:** la ejecución completa del juego original y una carrera física en PS Vita todavía NO se han verificado. No es un VPK final ni cambia la estimación histórica del porcentaje.


## Avance 136: texturas RGB565/RGBA y UV originales hacia vitaGL

- El invitado ARM Zeebo invoca las operaciones originales de creación,
  selección, configuración y carga de texturas de `GL_LEGACY`. Las cargas
  soportadas (`RGBA8` y `RGB565`, hasta 512×512) llegan al backend
  real vitaGL, sin rutas de éxito ficticias.
- `glTexCoordPointer` y los arreglos de coordenadas UV quedan conectados
  a `DrawArrays` y `DrawElements`, incluidos triángulos, tiras y abanicos.
- **39/39 pruebas sin ROM aprobadas** en
  [GitHub Actions](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38208510285).
- Este bloque todavía no significa que el juego original ya arranque ni
  sea jugable en una consola real. Faltan ejecutar los eventos del applet,
  enlazar archivos y sonido originales, completar llamadas gráficas,
  optimizar y verificar hardware. El 45.9% anterior es un estimado
  histórico, no porcentaje de jugabilidad certificado.
- [Informe técnico de texturas y límites](docs/AVANCE_135_ZEEBX_TEXTURAS_VITA.md).

## Avance 135: tiras y abanicos 3D del Zeebo

La ejecución gráfica de Zeebx ahora acepta `GL_TRIANGLES`,
`GL_TRIANGLE_STRIP` y `GL_TRIANGLE_FAN` de `glDrawArrays` y
`glDrawElements`, con índices reales del juego original en memoria
ARM virtual. Los triángulos quedan orientados correctamente para
la GPU de Vita, con límites de memoria y pruebas de errores antes
de enviar un dibujo.

- [Informe técnico y pruebas del avance 135](docs/AVANCE_135_ZEEBX_GL_TIRAS_ABANICOS.md).
- CI sin ROM: **37 pruebas**, incluida la conversión de primitivas.
- **El juego completo sigue sin probarse físicamente** y faltan
  texturas, UV, ejecución sostenida de applet, audio y archivos.


## Avance 134: matrices y proyecciones originales de Zeebx en vitaGL

- Se adaptaron `glFrustumx`, `glOrthox`, `glLoadMatrixx` y
  `glMultMatrixx` del GL antiguo del Zeebo: la GPU Vita recibe las
  proyecciones y matrices del juego original con conversión 16.16.
- Los punteros del ARM invitado y dos argumentos de pila se verifican antes
  de llamar a la GPU. No se devuelven éxitos imaginarios cuando falla.
- **36 comprobaciones sin ROM**, incluyendo tanto llamadas del invitado como
  llamadas al adaptador de GPU usado en Vita.
- [Implementación, límites y pruebas](docs/AVANCE_134_ZEEBX_GL_PROYECCION_MATRICES.md).
- **No significa que Crash original sea jugable todavía**: faltan texturas,
  más geometría y el ciclo continuo del applet; la prueba física es pendiente.
  No se ha aumentado artificialmente el porcentaje histórico de 45.9 %.


## Avance 133: estados gráficos del OpenGL original en vitaGL

- El puente Zeebx ejecuta ahora las llamadas originales BREW IGL para
  profundidad, transparencias, orientación y descartado de caras, color fijo
  16.16 y máscara de canales mediante callbacks **reales de vitaGL**.
- Los códigos de operación y parámetros proceden del contrato antiguo
  `GL_LEGACY` de Zeebx. No se aceptan llamadas fuera de memoria,
  enumeraciones no válidas ni resultados falsamente exitosos del GPU.
- Se mantienen intencionalmente detenidas las texturas originales,
  las coordenadas UV y los materiales hasta completar su adaptación.
  **Aún no es jugable ni se ha probado en una PS Vita física.**
- La suite portátil incluye **34 pruebas sin ROM**.
  [Ver avance técnico](docs/AVANCE_133_ZEEBX_ESTADOS_GL_VITA.md).

## Avance 132: dibujo de triangulos indexados desde Zeebo

La ruta experimental Zeebx ahora acepta `glDrawElements` con indices
originales de 8 y 16 bits, leyendo y validando toda la memoria ARM
invitada antes de enviar triangulos a la GPU. `glDrawArrays` usa la
misma ruta protegida. No se inventan vertices ni recursos originales.

Las pruebas no usan ROM comercial. El hecho de que el puente compile
no significa que el menu o una carrera original sean ya jugables.

[Detalles del avance 132](docs/AVANCE_132_TRIANGULOS_INDEXADOS_ZEEBX.md).

## Avance 131: triangulos originales de memoria guest a vitaGL

El puente experimental original de Zeebx reconoce glVertexPointer,
glEnableClientState, glDisableClientState y glDrawArrays para
GL_FLOAT XYZ con triangulos y limites estrictos de memoria guest.
No usa geometria inventada ni convierte punteros guest en
direcciones directas de PS Vita. Las coordenadas validadas pasan a
llamadas reales de dibujo de vitaGL. Se conservan todos los controles
y el motor nativo previamente comprobados.

[Detalles y limitaciones](docs/AVANCE_131_GEOMETRIA_TRIANGULOS_ZEEBX.md).

**Importante:** no se ha probado un modelo, una pista ni una carrera
original en la consola. Sigue sin existir VPK final instalable; las
pruebas sinteticas y VitaSDK no equivalen a jugabilidad completa.

## Avances 129-130: GPU real y matrices del Zeebo

- Se enlazo la primera salida GL original a vitaGL con EGL, glClear,
  glClearColorx, glViewport y presentacion de imagen (avance 129).
- Se anadieron glMatrixMode, glLoadIdentity, glPushMatrix, glPopMatrix,
  glTranslatex, glScalex y glRotatex mediante llamadas a GPU reales
  (avance 130, sometido a pruebas); argumentos firmados 16.16.
- No se inventa exito: las llamadas sin contexto o backend funcional se
  detienen. Los recursos privados del juego no se suben a GitHub.
- Siguen pendientes modelos, triangulos, texturas, estados GL, ciclo de
  eventos, audio y una carrera real probada en consola. Esto NO es aun
  el juego completo ni un VPK final.

[Puente vitaGL](docs/AVANCE_129_ZEEBX_GL_VITA_REAL.md) |
[Matrices 3D](docs/AVANCE_130_ZEEBX_MATRICES_VITA.md).

## Avance 141: recorte original de Zeebo a la pantalla Vita

El código derivado de Zeebx ahora pasa **glScissor** (slot 66 de la ABI original GL_LEGACY) al renderizador real de PS Vita. `GL_SCISSOR_TEST` se habilita y deshabilita por la ABI del juego. Se convierte el lienzo Zeebo 640×480 al viewport 725×544 centrado de Vita y se validan errores del GPU y límites de recorte. [Pruebas y límites](docs/AVANCE_141_ZEEBX_SCISSOR_VITA.md).

**43/43 pruebas de PC sin ROM aprobadas** en el bloque: [GitHub Actions](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38212299448). Las pruebas de PC y compilar un ELF ARM no demuestran que **Crash Nitro Kart 3D original sea jugable**. Falta ejecución completa de los gráficos, applet, archivos, sonido y validación en consola real; porcentaje histórico sin actualizar hasta tener esa evidencia.

## Avance 128: negociación EGL original de Zeebo

- Se conserva la rama de señales BREW del avance 127. Se añadió desde
  Zeebx la ABI del **EGL antiguo** que usa `cnk2.mod`: pantalla,
  `eglInitialize`, `eglQueryString`, elección y consulta de
  configuración 640×480 RGB565, errores y limpieza de estado.
- La selección de atributos valida el quinto argumento AAPCS en la
  pila virtual y nunca permite direcciones fuera de la memoria del juego.
- No inventamos contexto OpenGL ES ni imágenes: `eglCreateContext`,
  `eglMakeCurrent` y `eglSwapBuffers` siguen detenidos hasta que
  exista un GPU Vita funcional para las llamadas originales.
- Las pruebas de PC se ejecutan sin ROM; el juego completo aún no
  es jugable ni está probado físicamente en PS Vita.
- [Detalle técnico del avance 128](docs/AVANCE_128_ZEEBX_EGL_LEGACY.md).

Repositorio **privado** del port nativo de Zeebo a PlayStation Vita. **Todavía no es un videojuego completo ni se ha verificado su ejecución en consola.**

## Avance 127: callbacks originales de señales BREW

Se incorporó la fábrica `ISignalCBFactory::CreateSignal` con el quinto
parámetro del ABI ARM en la pila, objetos guest `ISignal`/`ISignalCtl`,
registro de eventos del mando `RegisterForButtonEvent` y
`RegisterForPositionChange`, y cola de avisos acotada. La primera
lectura de posición se notifica incluso con el analógico inmóvil.

`cnk_zeebx_arm_deliver_signals` interpreta los callbacks originales
**solo en un punto seguro de retorno del applet**, con límite de
instrucciones y restauración de registros. Los errores de API, direcciones
inválidas y desbordamientos se detienen explícitamente. Se enlazó la
entrega al diagnóstico opcional del applet y se reportan los avisos
realmente atendidos, sin declarar por ello un juego jugable.

**31 pruebas portátiles sin ROM** y compilación VitaSDK se comprueban
por GitHub Actions. Los tests ejercitan ARM sintético, no la carrera
completa del juego original.

## Avances 125-126: puente Zeebx HID y entrada física Vita (experimental)

- **Se conserva el motor de carreras nativo.** En el puente de ejecución
  Zeebx se reconocen las interfaces BREW del juego original: IDisplay,
  IFileMgr, ISound, IHID, señales y EGL/GL antiguos. Los métodos de GPU
  y audio no portados siguen deteniendo el intérprete; no hay respuestas
  de éxito inventadas.
- El dispositivo IHID informa un control original de Zeebo. La lectura
  de PS Vita alimenta los 18 índices de botones y cuatro ejes; la
  interfaz **IHIDDevice** responde a enumeración, estado de botón,
  posiciones, UIDs, mínimos/máximos y eventos FIFO de pulsar/soltar.
  Los eventos se entregan por orden y tiempo virtual, y reportan
  cuando la cola pierde eventos por saturación.
- El probe original se ejecuta únicamente mediante activación de
  diagnóstico; todavía NO alimenta una carrera original por cuadro.
  **Un ELF o VPK diagnóstico NO es un juego comercial recuperado.**
- **CI sin ROM: 30/30**:
  [pruebas de entrada y memoria](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38200754553).
  **VitaSDK ELF**:
  [enlace ARM](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38200777136).
- El VPK diagnóstico generado públicamente en GitHub **no incluye**
  archivos de Crash. Solo una compilación **privada y local**, con
  `CNK_PRIVATE_GAME_ASSETS_DIR` apuntando a los archivos originales del
  propietario, permite incorporarlos al mismo paquete. No publicar ni
  redistribuir esos datos sin permiso.

Pendiente para un verdadero **instalar y jugar**: interfaz gráfica
original a vitaGL, ejecución y programación continuas del applet,
entrada viva, archivos, sonido, recursos originales completos, pruebas
físicas y derechos de distribución.

## Avances 123-124: CPU ARM/BREW y pantalla virtual original

**Zeebx está siendo adaptado efectivamente al port, sin borrar nuestro
motor nativo de carreras.** El submódulo conserva íntegra la fuente
GPL de Zeebx para extender el puente de ejecución donde haga falta.

- **123:** adaptadas las clases BREW originales IDisplay, IFileMgr e
  ISound, `IShell::GetDeviceInfo` Zeebo 640×480/16 bpp, `memset`,
  `dbgprintf`, reloj virtual `GetTimeMS/GetUpTimeMS` y la fábrica
  `CreateInstance`. El objeto de pantalla permanece persistente.
- **124:** `IBitmap::QueryInterface` reconoce el `IDIB` real y
  reserva 614,400 bytes RGB565 de memoria **guest**, con `pBmp`,
  dimensiones, pitch, color scheme y referencias coherentes.
- **29/29 pruebas sin ROM aprobadas**:
  [validación](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38196593506).
  Estas pruebas ejecutan instrucciones ARM reales sobre datos de juego
  **sintéticos**, sin redistribuir el juego comercial.
- **VitaSDK ARM (avance 124)**:
  [ver compilación y enlace](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38196723001).
- Detalle técnico:
  [123](docs/AVANCE_123_FABRICAS_Y_DISPOSITIVO_ZEEBX.md) y
  [124](docs/AVANCE_124_ZEEBX_BITMAP_IDIB_RGB565.md).

El último examen privado del `cnk2.mod` original con un intérprete
**Python de referencia** identificó la siguiente clase pedida en la
creación del applet: `0x01014bc4` (**EGL antiguo de BREW**), después
de unas 6,444 instrucciones ARM de esta etapa. Esa traza **NO** es una
prueba del binario C en PS Vita física.

**Pendiente:** conectar el EGL/OpenGL ES de Zeebx al renderizador Vita,
completar APIs BREW, generar gráficos reales en pantalla y comprobar
una carrera completa en hardware. La pantalla RGB565 guest todavía
no se muestra en la consola; sigue sin haber SELF o VPK, y el
porcentaje anterior era una estimación histórica, no un certificado.

## Avance 121: emulador Zeebx completo disponible y ejecución ARM real

**El port nativo de PS Vita sigue intacto.** Se añadió como parte del
mismo repositorio el código fuente completo de
[ZeebxTeam/zeebx-emu](https://github.com/ZeebxTeam/zeebx-emu)
(submódulo Git `third_party/zeebx_emulator`, revisión fija
`f6d3e7bd6b27ca7f02d0e6f76a40a3c3f45d1cd9`).
Para traer los archivos del submódulo al clonar:
```bash
git submodule update --init --recursive
```

Además, adaptamos un **subconjunto ejecutable** del intérprete ARM1176,
el mapa de memoria y los trampolines BREW de Zeebx a C99
(`cnk_zeebx_arm.[ch]` y `cnk_zeebx_probe.[ch]`).
El código ARM invitado se interpreta dentro de memoria virtual, sin
ejecutarlo como instrucciones nativas de Vita. Los servicios
`malloc` y `IShell::AddRef` tienen respuestas de arranque comprobadas;
las APIs desconocidas detienen el intérprete y generan diagnóstico.

Un análisis **local de referencia**, usando el `cnk2.mod` original privado
del usuario (no incluido), verificó **21185 instrucciones ARM** hasta
el retorno de `AEEMod_Load`, con puntero real a `IModule*`.
No es todavía ejecución de carrera, gráficos BREW o eventos de applet,
ni una prueba del nuevo ejecutable C contra el archivo original en
GitHub Actions. El intérprete original completo de Zeebx no se ha
compilado sin cambios para VitaSDK.

- **24/24 tests portátiles sin ROM aprobados**:
  [GitHub Actions](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38194631197).
- **Compilación ARM de diagnóstico (no instalable)**:
  [VitaSDK](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38194631198).
- [Informe del avance y límites](docs/AVANCE_121_EJECUCION_ZEEBX_ARM_MOD.md).

El modo diagnóstico en Vita se activa únicamente creando el
archivo vacío `ux0:data/cnk3d/zeebx_probe.flag`; lee
`ux0:data/cnk3d/cnk2.mod` localmente y escribe
`ux0:data/cnk3d/zeebx_probe.txt`, **después sigue el mismo motor
nativo**. Por defecto permanece desactivado. No se ha generado
SELF ni VPK, ni se han publicado archivos comerciales del juego.
La parte adaptada de Zeebx está sujeta a su licencia GPL-2.0-or-later.

## Avance 120: lectura del ejecutable BREW gracias a Zeebx

- **Todo el port nativo C existente se conserva.** No se reemplazan
  el motor de carreras, la física, los circuitos ni el renderizador.
- Se adaptaron los lectores de `.mod` y `.mif` de
  [Zeebx](https://github.com/ZeebxTeam/zeebx-emu) a C99,
  con atribución GPL-2.0-or-later en `third_party/zeebx/NOTICE.md`.
- Datos de la copia original privada del usuario: `cnk2.mod` es un
  módulo ARM RAW con salto inicial a `0x14`; `274214.mif` declara
  ClassID `0x01081984`. Los binarios **no** se suben a GitHub.
- Lector independiente `cnk_inspeccionar_mod_zeebx` para explorar
  los archivos originales fuera del repositorio.
- **21/21 pruebas portátiles sin ROM aprobadas**:
  [38193191582](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38193191582).
- **Compilación y enlace ARM VitaSDK aprobados**:
  [38193191572](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38193191572).
- Ver [detalle de desarrollo](docs/AVANCE_120_ZEEBX_BREW_FORMATO_EJECUTABLE.md).

**Importante:** conocer un módulo ARM y su ClassID no significa que
el juego original ya corra en PS Vita. Sigue siendo un port nativo en
desarrollo, sin validación de carrera completa en hardware y sin SELF/VPK.

## Avance 119: primeros sprites originales visibles en Vita

- **Logo original de Crash Nitro Kart 3D:** al seleccionar circuito,
  el código GPU intenta mostrar la imagen real del juego que permanece
  exclusivamente en el `data.vfs` privado del usuario (FUFS 219,
  ID `89b14497`, 326×158, CRC32 `0c1897b1`).
- **Icono original Wumpa:** durante la carrera, junto al contador
  existente del HUD nativo (FUFS 119, ID `43bdcefe`, 32×32,
  CRC32 `5283e7a0`).
- Ambas imágenes usan **magenta opaco como fondo**. El port ahora aplica
  transparencia de clave cromática únicamente a esos PNG comprobados.
  En ROMs que no coincidan, continúa funcionando la interfaz de texto.
- **20/20 pruebas sin ROM en GitHub**:
  [38192490680](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38192490680).
- **Compilación ARM VitaSDK / ELF de diagnóstico**:
  [38192490699](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38192490699).
- Documento: [AVANCE_119_UI_ORIGINAL_PNG_NATIVO](docs/AVANCE_119_UI_ORIGINAL_PNG_NATIVO.md).

Los PNG **no están en GitHub**. Se cargan del FUFS original de la
persona usuaria; no hay VPK ni SELF. La imagen del juego completo,
la jugabilidad en hardware y la fidelidad de las pistas NO se
consideran probadas. La cifra de 45.9 % es solo una estimación histórica.

## Avances 117-118: reutilización real de Zeebx y preparación de texturas

- **Avance 117 (código integrado):** port a C99 del decodificador IMA ADPCM de
  [ZeebxTeam/zeebx-emu](https://github.com/ZeebxTeam/zeebx-emu) (GPL-2.0-or-later).
  `cnk_wav_asset_load` admite ahora WAV PCM y WAV IMA ADPCM a través del motor
  de audio nativo. Se conservan la licencia y el aviso del código adaptado.
  **Los 29 WAV encontrados en el archivo original examinado eran PCM:** el
  códec es soporte adicional, no una recuperación demostrada de audio CNK.
- **Avance 118 (herramienta integrada):**
  `scripts/generar_texturas_desde_cache_zeebx.py` crea filas
  `verified_textures.tsv` compatibles con el renderizador Vita a partir
  de una copia **local, propiedad del usuario** de `data.vfs` y archivos PNG
  originales extraídos por Zeebx. Solo acepta coincidencias de bytes exactas
  y únicas. Sin el caché original externo, ninguna asociación de textura
  nueva se considera verificada.
- **19/19 tests sin ROM en GitHub**:
  [ejecución 38191556688](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38191556688).
- **Compilación y enlace ARM VitaSDK aprobados**:
  [ejecución 38191556755](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38191556755).
- Detalles técnicos:
  [117](docs/AVANCE_117_ZEEBX_DECODIFICADOR_ADPCM.md) y
  [118](docs/AVANCE_118_ZEEBX_COINCIDENCIA_TEXTURAS.md).

**Alcance:** port nativo en desarrollo, NO juego completo ni probado en
PS Vita física. No se ha generado SELF ni VPK, y ningún recurso original
ha sido subido al repositorio. La última cifra de 45.9 % sigue siendo
una **estimación histórica**, no jugabilidad verificada.

## Avance 113 (11 de octubre de 2026)

- Selección de `data.vfs` externo con **verificación ligera de la tabla FUFS**: comprueba encabezado, límites, recuento y referencias antes de elegir una ruta. Evita que un archivo corrupto de mayor prioridad bloquee otra copia válida.
- **132/132 pruebas de PC aprobadas**, incluida la resistencia de carreras; el test FUFS también pasó con ASan/UBSan.
- **13 pruebas automáticas sin ROM** aprobadas en GitHub: [38187660896](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38187660896).
- **Compilación y enlace ARM VitaSDK** aprobados: [38187660904](https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38187660904).
- Ver `docs/AVANCE_113_VALIDACION_TABLA_FUFS.md`. **El juego no se ha probado todavía en una PS Vita real, no hay VPK y el porcentaje anterior sigue siendo solo estimado.**

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
