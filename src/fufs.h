/* FUFS/PLZP reader for Polarbit/Zeebo data.vfs; portable C99 + zlib.
 * Research component, not an ARM/BREW executor.
 */
#ifndef CNK_FUFS_H
#define CNK_FUFS_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t offset, id, size; } fufs_entry;
typedef struct { unsigned char *data; size_t length; uint32_t count; fufs_entry *entries; } fufs_file;
int fufs_open(const char *path, fufs_file *out);
int fufs_extract(const fufs_file *f, uint32_t index, unsigned char **out, size_t *size);
void fufs_close(fufs_file *f);
const char *fufs_type(const unsigned char *data, size_t len);
#endif
