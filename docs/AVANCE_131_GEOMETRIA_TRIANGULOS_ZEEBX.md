# Avance 131: primeros triangulos de vertice original al puente vitaGL

## Integracion real de geometria

- El interprete Zeebx GL_LEGACY ahora reconoce las posiciones documentadas
  de glVertexPointer (78), glEnableClientState (29),
  glDisableClientState (25) y glDrawArrays (26).
- La primera ruta valida vertices en memoria virtual del MOD original:
  GL_VERTEX_ARRAY, GL_FLOAT, componentes XYZ=3, stride entre 12 y
  256 bytes (o 0), GL_TRIANGLES y hasta 384 vertices por llamada.
- La lectura respeta limites exactos de modulo/stack/heap, rechaza
  direcciones desalineadas, conversiones que desbordan y valores no finitos.
  No se convierten punteros invitados a punteros fisicos de Vita.
- Un buffer de copia temporal guarda el bloque completo antes de la
  llamada al host. El host real envia glBegin(GL_TRIANGLES),
  glVertex3f() y glEnd() a vitaGL; consulta glGetError y transmite
  fallos de GPU al interprete. No se informa exito de un dibujado fallido.
- Se cuenta el numero de vertices enviados solamente al tener exito;
  todos los otros metodos OpenGL ES no implementados siguen bloqueados.

## Pruebas y limites

- Prueba sintetica sin ROM: triangulo de coordenadas reales, primer
  vertice desplazado, stride con relleno, habilitacion de array, memoria
  invalida, coordenadas NaN, modo/tipo incompatible, presupuesto
  excesivo y rechazo del backend de dibujo.
- Ruta optativa de investigacion del ejecutable Zeebo; **no es** la
  ejecucion por cuadro de la carrera original. La existencia del puente
  de triangulos no prueba que la geometria ni texturas comerciales se
  muestren en una PS Vita real.
- Pendiente: arrays de color, normales y textura, indices DrawElements,
  GL states, datos de origen, flujo IApplet::HandleEvent, audio y
  validacion en consola con activos privados proporcionados legalmente.
- Sin ROM ni recursos propietarios en GitHub. No se genera VPK final.
