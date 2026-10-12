#ifndef CNK_ZEEBX_ARM_H
#define CNK_ZEEBX_ARM_H
#include <stddef.h>
#include <stdint.h>
#include "cnk_zeebx_brew.h"

/* Independently C99-adapted subset of ZeebxTeam ARM1176 interpreter and
 * Zeebx BREW memory/trampoline bootstrap; GPL-2.0-or-later.
 * Isolated supplementary research path. It NEVER replaces native gameplay,
 * draws frames, or executes guest ARM as PS Vita native instructions.
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
 size_t module_size;
 int initialized,applet_pending;
} cnk_zeebx_arm;

/* initialize exact Zeebx bootstrap address regions; executable MOD is
 * user-provided and remains private (not serialized into this repo).
 * this optional module uses only ~3 MB, not Zeebx's 64MB heap. */
int cnk_zeebx_arm_init(cnk_zeebx_arm *cpu,const uint8_t *mod,size_t bytes);
void cnk_zeebx_arm_destroy(cnk_zeebx_arm *cpu);
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
