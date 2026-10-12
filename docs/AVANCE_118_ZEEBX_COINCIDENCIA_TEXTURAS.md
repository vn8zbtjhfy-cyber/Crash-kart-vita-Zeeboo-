# Avance 118: asociaciones de texturas con prueba byte a byte desde Zeebx

## Problema que resuelve

El port nativo ya contiene un renderizador de circuitos y un validador de
`verified_textures.tsv`. Sin ese archivo, los nombres originales `TXTR` de
cada `PIFF/SCEN` no prueban cuál de los cientos de PNG del archivo
`data.vfs` les corresponde. Asignar imágenes por proximidad o por orden
podría colocar una textura incorrecta en la pista.

Los registros externos de Zeebx enumeran rutas originales como
`data/textures/world/...` y `data/textures/carts/...`:
[registro de Crash Nitro Kart en Zeebx](https://github.com/ZeebxTeam/zeebx-emu/blob/development/docs/varredura/crash-bandicoot-nitro-kart-3d--brazil---es-pt.txt).

## Herramienta nueva

`scripts/generar_texturas_desde_cache_zeebx.py` funciona **en la computadora
del usuario**, no en Vita. Le entregamos dos entradas **privadas y locales**:

1. El `data.vfs` original de Zeebo que posee el usuario.
2. Una carpeta de recursos que *Zeebx ya haya extraído* del mismo juego
   (no el código fuente de Zeebx ni su carpeta genérica de iconos).

Uso con Python 3:

```bash
python3 scripts/generar_texturas_desde_cache_zeebx.py \
  --vfs "/ruta/a/data.vfs" \
  --zeebx-cache "/ruta/a/cache-del-juego-extraido" \
  --out "/ruta/a/verified_textures.tsv"
```

Solo crea filas del manifiesto si se dan TODAS estas condiciones:

- El `SCEN/TXTR` contiene el nombre de la imagen.
- Hay un archivo .png con ese nombre en la carpeta extraída de Zeebx.
- Sus **bytes son exactamente iguales** a los de un PNG extraído por el
  lector `FUFS/PLZP` original del proyecto (SHA-256 como índice, comparación
  de bytes como confirmación).
- El contenido corresponde a **un único** recurso PNG de `data.vfs`.
- El nombre no es ambiguo en el caché ni supera los límites del manifiesto.

El TSV generado tiene los seis campos exactos que consume
`cnk_texture_manifest_read`. El port vuelve a verificar `scene`, IDs,
PNG y correspondencias antes de subir texturas a GPU.

En ausencia de coincidencias seguras **no genera el archivo**. No
sobrescribe un archivo de asociaciones ya existente.

## Validación y límites

`tests/test_zeebx_cache_texture_manifest.py` crea una copia sintética de
FUFS, PNGs válidos, recurso PLZP comprimido y nombres SCEN. Verifica
coincidencias byte-exactas, duplicados, recursos inválidos y que no se
sobrescriban manifests.

Todavía no hay un caché Zeebx ni una ROM original del usuario presente
en esta sesión de CI: **no afirmamos haber identificado ninguna textura
real del juego**. Algunas instalaciones de Zeebx podrían no conservar
los PNG sin modificaciones; en ese caso las comparaciones fallarán
cerradas, sin inventar vínculos. El manifiesto resultante necesita revisión
visual en consola física antes de llamarlo fiel.

No subimos a GitHub PNG originales, `data.vfs`, datos del juego, ni
creamos SELF o VPK. Esta herramienta es original y solo emplea la
salida local de Zeebx; el único código GPL adaptado permanece en
`src/third_party/zeebx_ima.c`, con su aviso separado.
