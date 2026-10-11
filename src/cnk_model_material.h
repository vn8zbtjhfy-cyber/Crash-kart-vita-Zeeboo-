#ifndef CNK_MODEL_MATERIAL_H
#define CNK_MODEL_MATERIAL_H
#include "cnk_model.h"
#include "cnk_model_texl.h"
#include "fufs.h"
/* Conserves original 3DOB POLY material selectors, which refer to TEXL
 * entries by 1-based slot on inspected assets. This resolves a SOURCE
 * FILENAME, never a FUFS resource ID or a character identity.
 * Material 0 means no named TEXL texture (e.g. collision/auxiliary models).
 * Mixed TEXL #70 has a recognized but uninterpreted binary suffix;
 * its original texture filenames may now be read without inventing data. */
typedef struct {
 uint32_t original_selector;
 char source_png[CNK_TEXL_NAME_CAP]; /* empty when original_selector==0 */
} cnk_material_reference;
/* Audit-only: preserve material selectors with no TEXL name as unresolved.
 * Never use this partial result as a historically verified render binding.
 * The strict resolver below continues to reject such models atomically. */
int cnk_model_material_inspect(const cnk_model *model,
                                const cnk_model_texl *texl,
                                cnk_material_reference *dest,size_t dest_count,
                                unsigned *unresolved);
int cnk_model_material_resolve(const cnk_model *model,
                                const cnk_model_texl *texl,
                                cnk_material_reference *dest,
                                size_t dest_count);
int cnk_model_material_from_vfs(const fufs_file *vfs, uint32_t index,
                                 cnk_material_reference *dest,
                                 size_t dest_count,
                                 unsigned *group_count);
#endif
