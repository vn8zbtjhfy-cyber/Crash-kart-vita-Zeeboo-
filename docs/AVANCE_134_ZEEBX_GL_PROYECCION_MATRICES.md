# Avance 134: proyecciones y matrices GL originales del Zeebo hacia Vita

## Código implementado

Se amplía el puente experimental que ejecuta el `cnk2.mod` privado
utilizando la ABI BREW original de Zeebx, sin sustituir el motor
nativo de carreras ya existente.

Los métodos de `IGL` Common-Lite (`src/brew/aee_slots.rs` de
Zeebx, origen GPL-2.0-or-later) ahora despachan:

- **35** `glFrustumx`: seis valores 16.16 para la proyección en perspectiva.
- **47** `glLoadMatrixx`: matriz original de 16 valores fijos, columna mayor.
- **52** `glMultMatrixx`: multiplica la matriz del juego.
- **56** `glOrthox`: seis valores 16.16 para proyección ortográfica.

El lector interpreta los **16 elementos reales de memoria invitada**,
sin pasar punteros del juego a la GPU. Comprueba alineación y el rango
completo de 64 bytes *antes* de invocar el backend. Los argumentos quinto
y sexto de las proyecciones se leen como ARM AAPCS desde `SP` y `SP+4`,
con comprobación de límites y sin aceptar volúmenes degenerados.

`src/cnk_vita_zeebx_gl.c` envía el resultado al **vitaGL real** por
`glLoadMatrixf`, `glMultMatrixf`, `glFrustumf` y `glOrthof`. Ante un error
real de GL, un contexto no iniciado o un método sin backend, el intérprete
se **detiene**, sin simular una imagen dibujada.

## Verificación

- `zeebx_gl_projection`: pruebas de llamadas invitadas, matrices 16.16
  con signo, orden de memoria, seis parámetros en la pila ARM,
  rechazos de memoria inválida, volúmenes degenerados y GPU fallida.
- `zeebx_vitagl_projection`: compila el adaptador Vita real con símbolos
  gráficos sustitutos y comprueba que **llama de verdad** a las cuatro
  funciones GL y transmite argumentos correctos.
- Pruebas portátiles sin archivos de la ROM; CI incluye **36 pruebas**.
- El enlace ARM VitaSDK se comprueba por un flujo independiente.
  No hay prueba física de pantalla o carrera.

## Qué sigue

**Aún no hay juego completo**: falta portar carga de texturas y coordenadas
UV originales, ampliar los modos de geometría, implementar archivos,
audio y temporizadores BREW, y correr el applet y sus eventos de manera
continua. Se necesita una prueba de una carrera real en PS Vita.

El VPK de diagnóstico no equivale al VPK final. No se suben
`data.vfs`, `cnk2.mod`, `274214.mif` ni recursos comerciales.
El código derivado de Zeebx permanece sujeto a GPL-2.0-or-later.
El 45.9 % anterior es una **estimación histórica**, no una nueva
medición de jugabilidad verificada.
