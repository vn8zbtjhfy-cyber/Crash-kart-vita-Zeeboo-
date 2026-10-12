# Avance 132: geometria de vertices indexados en el puente Zeebx

## Cambio real

- Se implementa `GL_LEGACY::glDrawElements`, slot 27 de la tabla
  documentada por Zeebx, sin sustituir el motor nativo del proyecto.
- Se admiten solo `GL_TRIANGLES`, `GL_UNSIGNED_BYTE` (0x1401) y
  `GL_UNSIGNED_SHORT` (0x1403). Los indices se leen de la memoria ARM
  invitada original, con comprobaciones estrictas de direccion y alineacion.
- Los indices se resuelven a posiciones `GL_FLOAT` (XYZ) de la
  `glVertexPointer` verificada en avance 131, conservando su orden.
  Se limita cada envio a 384 vertices y el rango por indice a 0..65535.
- Una funcion compartida valida por completo y convierte los datos a
  floats propios del host antes de enviarlos a vitaGL. El motor nunca
  trata una direccion virtual del juego como un puntero nativo.
- Si los indices estan truncados, contienen referencias sin memoria,
  hay NaN, no hay contexto, o la GPU rechaza el envio, se detiene la
  invocacion de forma diagnostica. Solo se suman contadores tras un
  envio real aceptado por la GPU.
- `glDrawArrays` del avance 131 conserva su comportamiento y usa la
  misma ruta defensiva, reduciendo duplicacion de comprobaciones.

## Pruebas y alcance

- Pruebas ROM-free de indices de byte y uint16, secuencia de vertices,
  entradas mal alineadas, offsets fuera de memoria, tipo/mode invalidos,
  referencias fuera de rango, falta de callback y fallo del backend.
- CI en Linux comprueba el puente sobre memoria ARM de prueba. El
  enlace VitaSDK es otra prueba distinta; ninguno comprueba por si
  mismo una carrera comercial en la consola.
- No se importan recursos de Crash en GitHub ni se genera VPK final.
  Faltan UV y texturas, estados GL, applet original por cuadro, sonido
  y pruebas reales en PS Vita con archivos originales del propietario.
