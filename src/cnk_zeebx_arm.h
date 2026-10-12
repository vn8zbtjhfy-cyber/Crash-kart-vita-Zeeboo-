#ifndef CNK_ZEEBX_ARM_H
#define CNK_ZEEBX_ARM_H
#include <stddef.h>
#include <stdint.h>
#include "cnk_zeebx_brew.h"

/* Independently C99-adapted subset of ZeebxTeam ARM1176 interpreter and
 * Zeebx BREW memory/trampoline bootstrap; GPL-2.0-or-later.
 * Isolated supplementary research path. It NEVER replaces native gameplay
 * or executes guest ARM as PS Vita native instructions. Original guest GL
 * calls can render only when a real, initialized host GPU is explicitly
 * attached; default gameplay remains the separate native racing engine.
 */
#define CNK_ARM_MOD_BASE 0x00010000u
#define CNK_ARM_STACK_BASE 0x20000000u
#define CNK_ARM_HEAP_BASE 0x10000000u
#define CNK_ARM_HEAP_BYTES 0x1000000u
#define CNK_ARM_OBJECT_BASE 0x30000000u
#define CNK_ARM_HELPERS_BASE 0x31000000u
#define CNK_ARM_VTABLE_BASE 0xe0000000u
#define CNK_ARM_API_BASE 0xf0000000u
#define CNK_ARM_RETURN_MAGIC 0xfff00000u
#define CNK_ARM_MOD_PREFIX 0x1000u
#define CNK_ARM_STACK_BYTES 0x100000u
#define CNK_ARM_BSS_BYTES 0x100000u
#define CNK_ARM_VTABLE_STRIDE 0x400u
/* Zeebo original controller: 18 enumerated buttons, 4 absolute axes. */
#define CNK_ZEEBX_PAD_BUTTONS 18u
#define CNK_ZEEBX_PAD_AXES 4u
/* Maximum verified, bounded guest vertices submitted by one draw call. */
#define CNK_ZEEBX_MAX_DRAW_VERTICES 384u
#define CNK_ZEEBX_MAX_GL_BUFFERS 32u
#define CNK_ZEEBX_MAX_GL_BUFFER_BYTES (512u*1024u)
#define CNK_ZEEBX_MAX_GL_BUFFER_TOTAL (2u*1024u*1024u)
typedef struct {
 uint32_t name,size,ever_bound;
 uint8_t *data; /* source guest bytes, never executable host memory */
} cnk_zeebx_gl_buffer;

typedef struct {
 uint32_t buttons; /* bit per Zeebx input/mod.rs BUTTON_UIDS entry */
 uint8_t axes[CNK_ZEEBX_PAD_AXES]; /* absolute [0..255], neutral=128 */
} cnk_zeebx_pad;
/* Finite guest event FIFO. Only transitions from confirmed host samples
 * are queued; the guest reads them through GetNextButtonEvent slot 9. */
#define CNK_ZEEBX_HID_QUEUE 64u
typedef struct {
 uint32_t index,pressed,at_ms;
} cnk_zeebx_hid_event;
#define CNK_ZEEBX_MAX_SIGNALS 16u
#define CNK_ZEEBX_PENDING_SIGNALS 32u
typedef struct {
 uint32_t signal_obj,control_obj,callback,context;
 uint32_t signal_refs,control_refs;
 uint8_t enabled,attached;
} cnk_zeebx_signal;
typedef struct {
 uint32_t function,context;
} cnk_zeebx_signal_callback;

/* Explicit Vita GPU host services. A guest GL call must never claim it
 * rendered unless a real, initialized renderer accepts the request.
 * Callbacks return 0 for success; any other value traps the guest API. */
typedef struct {
 int (*make_current)(void *user,uint32_t width,uint32_t height);
 int (*clear_color)(void *user,float r,float g,float b,float a);
 int (*clear)(void *user,uint32_t mask);
 int (*viewport)(void *user,int32_t x,int32_t y,uint32_t w,uint32_t h);
 /* GLES1 scissor rect in original 640x480 framebuffer coordinates. */
 int (*scissor)(void *user,int32_t x,int32_t y,uint32_t w,uint32_t h);
 int (*present)(void *user);
 /* Zeebx GL_LEGACY matrix entries. Optional until implemented by host:
  * unsupported calls must TRAP, never silently return success. */
 int (*matrix_mode)(void *user,uint32_t mode);
 int (*load_identity)(void *user);
 int (*push_matrix)(void *user);
 int (*pop_matrix)(void *user);
 int (*translate)(void *user,float x,float y,float z);
 int (*scale)(void *user,float x,float y,float z);
 int (*rotate)(void *user,float degrees,float x,float y,float z);
 /* Original 16.16 Zeebo matrices and 6-argument projection APIs.
  * Host receives a temporary validated C float[16]; synchronous only. */
 int (*load_matrix)(void *user,const float matrix[16]);
 int (*mult_matrix)(void *user,const float matrix[16]);
 int (*frustum)(void *user,float left,float right,float bottom,
                float top,float near_z,float far_z);
 int (*ortho)(void *user,float left,float right,float bottom,
              float top,float near_z,float far_z);
 /* Zeebx AEEGL.h fixed-pipeline state, delegated to actual GPU.
  * Each pointer stays optional until host implements it; unknown or
  * unbound state changes must STOP the guest rather than fake success. */
 int (*set_capability)(void *user,uint32_t capability,int enabled);
 int (*blend_func)(void *user,uint32_t source,uint32_t destination);
 int (*depth_func)(void *user,uint32_t comparison);
 int (*depth_mask)(void *user,int enabled);
 /* Source GL_LEGACY slots 4, 59, 67: alpha-test, polygon depth
  * offset and shading mode. Never accept success without real GPU. */
 int (*alpha_func)(void *user,uint32_t compare,float reference);
 int (*polygon_offset)(void *user,float factor,float units);
 int (*shade_model)(void *user,uint32_t mode);
 int (*cull_face)(void *user,uint32_t face);
 int (*front_face)(void *user,uint32_t orientation);
 int (*color4)(void *user,float red,float green,float blue,float alpha);
 int (*color_mask)(void *user,int red,int green,int blue,int alpha);
 /* Real 3D guest vertex stream in XYZ float triplets, owned by caller
  * through return of this synchronous callback. No fake draw success. */
 int (*draw_triangles)(void *user,const float *xyz,uint32_t count);
 /* Source Zeebx GL_COLOR_ARRAY, normalized per-vertex RGBA8. The
  * host consumes these floats synchronously, without retaining guest RAM. */
 int (*draw_colored_triangles)(void *user,const float *xyz,
                                const float *rgba,uint32_t count);
 /* Guest texture commands are copied from bounded guest RAM BEFORE
  * invoking the host; no guest pointer escapes or is retained. */
 int (*texture_gen)(void *user,uint32_t count,uint32_t *names);
 int (*texture_delete)(void *user,uint32_t count,const uint32_t *names);
 int (*texture_bind)(void *user,uint32_t name);
 int (*texture_parameter)(void *user,uint32_t pname,uint32_t value);
 /* Crash requests GL_REPLACE with TexEnvxv (pointer), not only scalar.
  * Enums are raw GL integers even in the fixed-point x API. */
 int (*texture_env_mode)(void *user,uint32_t mode);
 int (*texture_upload)(void *user,uint32_t width,uint32_t height,
                       uint32_t format,uint32_t type,const uint8_t *pixels);
 /* A real GLES1 atlas update: pixels are packed guest texels copied
  * before the callback; host MUST reject invalid target rectangles. */
 int (*texture_sub_upload)(void *user,uint32_t x,uint32_t y,
                            uint32_t width,uint32_t height,
                            uint32_t format,uint32_t type,
                            const uint8_t *pixels);
 int (*draw_textured_triangles)(void *user,const float *xyz,
                                const float *uv,uint32_t count);
 int (*draw_colored_textured_triangles)(void *user,const float *xyz,
                                        const float *uv,const float *rgba,
                                        uint32_t count);
} cnk_zeebx_gl_host;

typedef enum {
 CNK_ARM_BUDGET=0, CNK_ARM_API=1, CNK_ARM_RETURNED=2,
 CNK_ARM_FAULT=3, CNK_ARM_UNSUPPORTED=4
} cnk_arm_reason;
typedef struct {
 cnk_arm_reason reason;
 uint32_t pc,opcode,address,api_interface,api_slot;
 uint64_t executed;
} cnk_arm_outcome;
typedef struct {
 uint32_t regs[16],cpsr;
 uint64_t executed;
 uint8_t *module,*stack,*heap,*objects,*helpers,*vtables;
 uint32_t heap_cursor,shell_refs;
 uint32_t object_cursor,display_objects,filemgr_objects,sound_objects,debug_calls;
 uint32_t hid_objects,signal_factory_objects,egl_legacy_objects,gl_legacy_objects;
 uint32_t screen_bitmap_addr,screen_bitmap_references;
 uint32_t screen_bitmap_pixels; /* RGB565 guest buffer, allocated on first IDIB */
 uint32_t virtual_time_ms;
 /* Zeebx legacy IEGL (AEEGL.h): guest-owned configuration only.
  * GL contexts/swap are not functional until the Vita GPU bridge exists. */
 uint32_t egl_error,egl_initialized,egl_queries;
 uint32_t egl_surface,egl_context,egl_current_surface,egl_current_context;
 uint32_t gl_clear_calls,gl_presents,gl_viewport_calls;
 uint32_t gl_vertex_pointer,gl_vertex_stride,gl_vertex_type;
 uint32_t gl_draw_calls,gl_drawn_vertices;
 uint8_t gl_vertex_enabled;
 uint32_t gl_uv_pointer,gl_uv_stride,gl_uv_type,gl_bound_texture;
 uint32_t gl_color_pointer,gl_color_stride,gl_color_type,gl_color_buffer;
 uint32_t gl_array_buffer,gl_element_buffer,gl_vertex_buffer,gl_uv_buffer;
 uint32_t gl_next_buffer_name,gl_total_buffer_bytes;
 cnk_zeebx_gl_buffer gl_buffers[CNK_ZEEBX_MAX_GL_BUFFERS];
 uint32_t gl_texture_uploads,gl_texture_sub_uploads,gl_textured_draw_calls;
 uint32_t gl_unpack_alignment; /* Zeebx glPixelStorei, default 4 */
 uint8_t gl_uv_enabled,gl_texturing_enabled,gl_color_enabled;
 cnk_zeebx_gl_host gl_host;
 void *gl_host_user;
 uint32_t egl_string_vendor,egl_string_version,egl_string_extensions;
 cnk_zeebx_pad pad; /* separate from the native driving physics */
 cnk_zeebx_hid_event hid_queue[CNK_ZEEBX_HID_QUEUE];
 uint32_t hid_queue_head,hid_queue_count,hid_events_dropped;
 cnk_zeebx_signal signals[CNK_ZEEBX_MAX_SIGNALS];
 cnk_zeebx_signal_callback signal_pending[CNK_ZEEBX_PENDING_SIGNALS];
 uint32_t signal_count,signal_pending_head,signal_pending_count;
 uint32_t hid_button_signal,hid_position_signal;
 char last_debug_template[128];
 size_t module_size;
 int initialized,applet_pending;
} cnk_zeebx_arm;

/* initialize exact Zeebx bootstrap address regions; executable MOD is
 * user-provided and remains private (not serialized into this repo).
 * this optional module uses only ~3 MB, not Zeebx's 64MB heap. */
int cnk_zeebx_arm_init(cnk_zeebx_arm *cpu,const uint8_t *mod,size_t bytes);
void cnk_zeebx_arm_destroy(cnk_zeebx_arm *cpu);
/* Host-supplied virtual elapsed milliseconds, as in Zeebx Machine::elapsed_ms.
 * Defaults to 0 at boot; never derived from guest instruction throughput.
 * The native app can advance this clock independently once integrated. */
int cnk_zeebx_arm_set_virtual_time(cnk_zeebx_arm *cpu,uint32_t elapsed_ms);
/* A snapshot of the actual Vita controller is passed from the host;
 * virtual BREW input never assumes a pressed button on its own. */
int cnk_zeebx_arm_set_pad(cnk_zeebx_arm *cpu,const cnk_zeebx_pad *pad);
/* Bind from Vita ONLY after vitaGL has initialized. Unbound guest GL
 * calls explicitly stop, not silently emulate a finished renderer. */
int cnk_zeebx_arm_set_gl_host(cnk_zeebx_arm *cpu,
                              const cnk_zeebx_gl_host *host,void *user);
/* Execute deferred guest ISignal callbacks ONLY at a safe guest return
 * boundary. Every callback is budgeted and preserves host guest-register
 * context. Unresolved calls stop explicitly; this is still experimental. */
int cnk_zeebx_arm_deliver_signals(cnk_zeebx_arm *cpu,uint32_t budget,
                                  cnk_arm_outcome *out,uint32_t *delivered);
/* Returns >=0 for an execution outcome, never runs indefinitely.
 * Caller must handle API outcome; no invented successful BREW replies.
 * Every iteration reuses saved registers, flags and memory state. */
int cnk_zeebx_arm_run(cnk_zeebx_arm *cpu,uint32_t budget,cnk_arm_outcome *out);
/* Caller-provided, explicitly verified result of last intercepted BREW
 * invocation; proceed at LR. Use only after CNK_ARM_API, never implicitly. */
int cnk_zeebx_arm_return_from_api(cnk_zeebx_arm *cpu,uint32_t result);
/* Bootstrap only: faithfully modeled Zeebx helper malloc (interface 6,
 * slot 26), IShell AddRef (interface 0, slot 0) and Release (slot 1).
 * Any unknown call remains an explicit BREW_API stop, NOT a dummy success. */
int cnk_zeebx_arm_handle_bootstrap_api(cnk_zeebx_arm *cpu);
/* Runs up to budget true ARM instructions, allowing only verified
 * bootstrap APIs to resume guest code. Unknown API or CPU opcode stops. */
int cnk_zeebx_arm_bootstrap(cnk_zeebx_arm *cpu,uint32_t budget,
                            cnk_arm_outcome *out,uint32_t *apis_handled);
int cnk_zeebx_arm_read_u32(cnk_zeebx_arm *cpu,uint32_t addr,uint32_t *out);
/* Stage 2 of Zeebx Machine::create_applet: after a confirmed successful
 * AEEMod_Load, read the REAL guest IModule vtable and invoke slot 2
 * CreateInstance(IModule*, IShell*, AEECLSID, IApplet**).
 * Returns <0 if guest pointers, bootstrap state, or code are invalid.
 * Zero indicates only that guest registers are ready, not that the applet
 * was created. The caller MUST run cnk_zeebx_arm_bootstrap again. */
int cnk_zeebx_arm_prepare_applet(cnk_zeebx_arm *cpu,uint32_t clsid,
                                  uint32_t *entry);
/* After the stage-2 interpreter returns, read its real output pointer;
 * success requires BREW result r0==0 AND nonzero IApplet*.
 * Returns <0 or 0, never allocates/fabricates an applet. */
int cnk_zeebx_arm_applet_result(cnk_zeebx_arm *cpu,
                                const cnk_arm_outcome *stop,
                                uint32_t *applet_ptr);
#endif
