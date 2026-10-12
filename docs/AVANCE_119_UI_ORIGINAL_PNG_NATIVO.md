# Avance 119: primeros gráficos auténticos del HUD y menú de Zeebo

**No es una imagen recreada ni un render de Infuse.** En los recursos
originales `data.vfs` que el usuario entregó fuera de GitHub se verificaron
visualmente los siguientes dos PNG con referencia exacta al recurso fuente:

| Papel observado en el PNG | Índice FUFS | ID FUFS | Tamaño PNG | CRC32 de bytes PNG extraídos |
|---|---:|---|---:|---|
| Logotipo original «Crash Bandicoot Nitro Kart 3D» | 219 | `89b14497` | 326x158 | `0c1897b1` |
| Icono de fruta Wumpa | 119 | `43bdcefe` | 32x32 | `5283e7a0` |

**Verificado:** metadatos, visualización de los dos originales y huella
CRC32 de las imágenes *extraídas de la copia privada del usuario*.
El nombre exacto de archivo del logotipo en el módulo Zeebo sigue sin
identificación demostrada; solo el papel visual es seguro.

## Cambio jugable

- `cnk_source_ui.[ch]` no contiene las imágenes: exige que los recursos
  de `data.vfs` coincidan en posición, ID, dimensión y CRC32 con las
  referencias comprobadas. No acepta una asociación basada en parecidos.
- `cnk_vitagl_ui_art.[ch]` decodifica únicamente esas imágenes desde el
  FUFS original mediante libpng y sube el RGBA a vitaGL.
- **Transparencia:** ambos PNG originales contienen un fondo **magenta
  opaco** (RGB 255,0,255) aunque no llevan chunk `tRNS`. Convertir
  exclusivamente esa clave a alfa 0 elimina el rectángulo rosa que
  dibujaría una carga convencional. No se aplica indiscriminadamente
  a los materiales del escenario.
- En el selector de pistas se dibuja el logotipo auténtico; en carrera,
  la fruta auténtica acompaña al contador Wumpa del HUD compacto.
- Los GPU handles son propiedad de la aplicación y se liberan al cerrar.
- Si se usa una variante distinta de la ROM o una imagen fue modificada,
  la validación falla cerrada y el **HUD de texto sigue funcionando**,
  sin fingir que los sprites tienen identidad verificada.
- No se altera ni el motor de carrera ni las asignaciones de texturas
  de circuito, que siguen requiriendo verificación independiente.

## Pruebas

- `source_ui`: sin ROM, con PNG 2x1 **sintético**; valida dimensiones,
  corrupción de CRC y color-key exacto, así como entradas inválidas.
- El mismo ejecutable acepta opcionalmente la ruta de `data.vfs`
  del usuario para validar ambos recursos originales localmente,
  pero esa prueba no se ejecuta en GitHub Actions (el CI no posee la ROM).
- El enlace de VitaSDK ARM, más las pruebas sin ROM, se comprueban
  antes de declarar finalizado este avance.

## Límites

El resto de los cientos de PNG sigue sin mapearse automáticamente al
renderizador. Este trabajo NO implica que haya menú completo original,
texturas originales de todos los circuitos, animación fiel, juego probado
en consola física ni VPK/SELF. El porcentaje histórico de 45.9 % no
debe crecer sin evidencia de jugabilidad real.
