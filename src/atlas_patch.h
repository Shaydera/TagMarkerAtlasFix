#ifndef TMAF_ATLAS_PATCH_H
#define TMAF_ATLAS_PATCH_H
#include <stddef.h>
#include <stdint.h>
enum AtlasResult { ATLAS_OK=0, ATLAS_NOT_FOUND=1, ATLAS_AMBIGUOUS=2, ATLAS_MISMATCH=3 };
int AtlasFind(const uint8_t *data, size_t size, size_t *offset);
int AtlasRewrite(uint8_t *site, size_t size, int restore);
#endif
