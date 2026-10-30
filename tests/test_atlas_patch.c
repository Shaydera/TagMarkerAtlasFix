#include "atlas_patch.h"
#include <stdio.h>
#include <string.h>
static const uint8_t original[]={
    0xc7,0x45,0x67,0,0,0x80,0x44,0xc7,0x45,0x6b,0,0,0x80,0x44,
    0xc7,0x45,0x6f,0,0,0x80,0x43,0xc7,0x45,0x73,0,0,0x80,0x43};
static int failures=0,checks=0;
#define CHECK(c,label) do { ++checks; if(!(c)) { ++failures; printf("FAIL: %s\n",label); } } while(0)
int main(void) {
    uint8_t data[96],saved[96]; size_t offset=777;
    memset(data,0xcc,sizeof(data)); memcpy(data+11,original,sizeof(original));
    CHECK(AtlasFind(data,sizeof(data),&offset)==ATLAS_OK && offset==11,"find sole sequence at an unaligned offset");
    CHECK(AtlasRewrite(data+11,sizeof(original),0)==ATLAS_OK,"apply atlas-only patch");
    for(size_t i=0;i<sizeof(original);++i) {
        uint8_t wanted=original[i];
        if(i==5 || i==12) wanted=0;
        if(i==6 || i==13) wanted=0x45;
        CHECK(data[11+i]==wanted,"change atlas immediates only; preserve tile size and instructions");
    }
    memcpy(saved,data,sizeof(data));
    CHECK(AtlasRewrite(data+11,sizeof(original),0)==ATLAS_MISMATCH,"do not apply over modified code");
    CHECK(memcmp(data,saved,sizeof(data))==0,"refusal leaves code unchanged");
    CHECK(AtlasRewrite(data+11,sizeof(original),1)==ATLAS_OK,"restore verified patched bytes");
    CHECK(memcmp(data+11,original,sizeof(original))==0,"restore exact original sequence");
    memcpy(data+60,original,sizeof(original)); offset=777;
    CHECK(AtlasFind(data,sizeof(data),&offset)==ATLAS_AMBIGUOUS,"refuse duplicate matches");
    CHECK(offset==777,"no candidate returned for ambiguous input");
    data[60]=0; data[11]=0; offset=777;
    CHECK(AtlasFind(data,sizeof(data),&offset)==ATLAS_NOT_FOUND,"refuse missing sequence");
    CHECK(offset==777,"no candidate returned for missing input");
    memcpy(data+68,original,sizeof(original)); offset=777;
    CHECK(AtlasFind(data,sizeof(data),&offset)==ATLAS_OK && offset==68,"match exactly at end of buffer");
    CHECK(AtlasFind(data,20,&offset)==ATLAS_NOT_FOUND,"short input cannot match");
    CHECK(AtlasFind(NULL,0,&offset)==ATLAS_NOT_FOUND,"empty input is safe");
    memcpy(saved,data,sizeof(data));
    CHECK(AtlasRewrite(data+68,27,0)==ATLAS_MISMATCH,"refuse truncated patch site");
    CHECK(memcmp(data,saved,sizeof(data))==0,"truncated site remains unchanged");
    data[68+6]=0x45;
    memcpy(saved,data,sizeof(data));
    CHECK(AtlasRewrite(data+68,28,1)==ATLAS_MISMATCH,"do not restore over foreign or partial modification");
    CHECK(memcmp(data,saved,sizeof(data))==0,"foreign modification remains unchanged");
    printf("%d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
