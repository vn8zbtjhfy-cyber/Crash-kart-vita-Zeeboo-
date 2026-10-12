# Avance 128: EGL antiguo del juego original hacia PS Vita

## Implementado en código fuente

- Adaptación C99 del **IEGL antiguo de AEEGL.h** a partir de Zeebx
  (GPL-2.0-or-later): funciones expuestas como punteros C, **sin**
  parámetro `this` (ABI diferente de IEGL11).
- `eglGetDisplay`, `eglInitialize`, `eglTerminate`, `eglGetError`,
  `eglGetConfigs`, `eglChooseConfig`, `eglGetConfigAttrib`,
  `eglQueryString` y consultas de contexto **inactivo**.
- Una configuración de la pantalla original 640×480, color RGB565,
  profundidad 16 y stencil 8, con valores de Zeebx y el SDK EGL del Zeebo.
- Comprobación de punteros y longitud de listas de atributos antes
  de escritura en memoria ARM virtual. El quinto argumento AAPCS de
  `eglChooseConfig` se toma de la pila virtual, no de registros inventados.
- Cadenas del entorno alojadas en memoria virtual real, sin fugas
  por consultas repetidas. Las extensiones NO implementadas no se anuncian.
- **Sin falsear el progreso**: `eglCreateContext`,
  `eglMakeCurrent`, `eglSwapBuffers`, carga de modelos y dibujo
  original aún se detienen en su método BREW concreto.
- Ningún archivo del juego comercial se subió al repositorio.

## Validación

- **31/31 pruebas host/CI sin ROM** aprobadas:
  https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-/actions/runs/38201900590
- Test nuevo `tests/test_native_zeebx_egl_legacy.c`: creación
  del objeto IEGL vía `IShell::CreateInstance`, vtable exacta, inicialización,
  formato RGB565, capacidad real, filtros de `ChooseConfig`, lectura
  de cadenas, error EGL, desreferencias incorrectas y bloqueos gráficos.
- La compilación y prueba en **hardware físico** sigue pendiente.

## Próximo objetivo imprescindible

1. Adaptar las llamadas esenciales de `IGL`/OpenGL ES 1.0 al GPU
   mediante vitaGL, con buffers y texturas originales y salida de video.
2. Procesar el `IApplet::HandleEvent` original con entrada por cuadro,
   audio/archivos BREW y sincronización real.
3. Verificar menú, carrera completa y recursos del juego en Vita física.

**No es una VPK final ni una prueba de que Crash ya corra.** El
motor de carreras nativo previo continúa intacto. Código Zeebx
adaptado sujeto a GPL-2.0-or-later.
