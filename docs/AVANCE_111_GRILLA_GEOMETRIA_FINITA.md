# Avance 111: rechazar límites geométricos que desbordan la grilla

Fecha: 12 de octubre de 2026.

## Defecto reproducido

Una geometría sintética con vértices **finitos** en `-FLT_MAX` y `FLT_MAX` era aceptada por `cnk_collision_grid_build`. La resta de extremos desbordaba a infinito (`dx=inf`), permitiendo divisiones `inf/inf` y conversiones no definidas de NaN a índices de celda. La prueba nueva falló contra el código previo (aserción: el constructor devolvió 0 en vez de rechazar la geometría).

## Corrección

Se rechazan anchos de celda no finitos, no positivos y límites calculados de la grilla no finitos **antes** de reservar memoria o indexar celdas. La grilla normal para las escenas de trabajo conserva sus operaciones.

## Validación

- PC Release con `data.vfs` privado de desarrollo: **131/131** pruebas CTest, incluidos circuitos y carrera multivuelta.
- Prueba nueva sin ROM `collision_grid_extents`: rechaza desbordamientos X/Z y mantiene la consulta de suelo con triángulos normales.
- La prueba focalizada con AddressSanitizer, UndefinedBehaviorSanitizer y `float-cast-overflow` pasa.
- Pendiente CI VitaSDK del commit publicado y pruebas en PS Vita física. El enlace ELF, incluso si pasa, no demuestra ejecución en consola.

**Sin SELF ni VPK, sin recursos originales añadidos al repositorio.** No se declara el juego completo ni se modifica artificialmente el progreso histórico.
