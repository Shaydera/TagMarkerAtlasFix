#include "atlas_patch.h"
static const uint8_t original[28]={
    0xc7,0x45,0x67,0,0,0x80,0x44,0xc7,0x45,0x6b,0,0,0x80,0x44,
    0xc7,0x45,0x6f,0,0,0x80,0x43,0xc7,0x45,0x73,0,0,0x80,0x43};
static uint8_t Expected(size_t index, int patched) {
    if(patched && (index==5 || index==12)) return 0;
    if(patched && (index==6 || index==13)) return 0x45;
    return original[index];
}
int AtlasFind(const uint8_t *data, size_t size, size_t *offset) {
    size_t match=0; int found=0;
    if(!data || !offset || size<sizeof(original)) return ATLAS_NOT_FOUND;
    for(size_t i=0;i<=size-sizeof(original);++i) {
        if(data[i]!=original[0]) continue;
        size_t j=1;
        while(j<sizeof(original) && data[i+j]==original[j]) ++j;
        if(j==sizeof(original)) {
            if(found) return ATLAS_AMBIGUOUS;
            found=1; match=i;
        }
    }
    if(!found) return ATLAS_NOT_FOUND;
    *offset=match;
    return ATLAS_OK;
}
int AtlasRewrite(uint8_t *site, size_t size, int restore) {
    if(!site || size<sizeof(original)) return ATLAS_MISMATCH;
    for(size_t i=0;i<sizeof(original);++i) {
        if(site[i]!=Expected(i,restore!=0)) return ATLAS_MISMATCH;
    }
    // Only the two float immediates change. The opcodes and tile sizes stay intact.
    for(size_t i=3;i<7;++i) site[i]=Expected(i,restore==0);
    for(size_t i=10;i<14;++i) site[i]=Expected(i,restore==0);
    return ATLAS_OK;
}
