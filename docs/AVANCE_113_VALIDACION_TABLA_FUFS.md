# Avance 113: validación ligera de la tabla FUFS antes del arranque

La búsqueda de la copia privada `data.vfs` comprueba ahora el directorio de entradas FUFS antes de aceptar una ruta. Rechaza un encabezado falso, tablas truncadas, recuentos excesivos, entradas que apuntan dentro de la tabla y tamaños que rebasan el archivo. Si una copia de mayor prioridad falla, examina la siguiente. Se utiliza lectura secuencial y una memoria constante (12 bytes por lectura), sin cargar la ROM en esta fase.

El parser completo `fufs_open()` sigue validando y cargando el archivo durante el inicio del juego. El cambio no asegura la integridad criptográfica ni verifica contenidos comprimidos: solo reduce los bloqueos evitables en el selector. No se copia `data.vfs` dentro del futuro VPK ni se publica ningún recurso original.

Pruebas: compilación de PC, 132/132 tests con los materiales proporcionados por el usuario y test específico del selector con ASan/UBSan. VitaSDK ELF y CI PC aprobados, ejecuciones 38187660904 y 38187660896. No se ha demostrado aún arranque o jugabilidad en una PS Vita física y no se genera VPK.
