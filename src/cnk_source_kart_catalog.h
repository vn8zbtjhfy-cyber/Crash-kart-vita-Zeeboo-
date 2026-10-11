#ifndef CNK_SOURCE_KART_CATALOG_H
#define CNK_SOURCE_KART_CATALOG_H
#include <stdint.h>
/* Immutable source identity: six Zeebo 3DOB kart families, three original LODs.
 * This describes model bytes, NOT character identity or original PNG lookup. */
int cnk_source_kart_gallery_source(unsigned family,unsigned lod,
                                   uint32_t *index,uint32_t *id);
#endif
