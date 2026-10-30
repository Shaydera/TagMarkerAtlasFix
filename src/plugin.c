/* Standalone marker atlas capacity fix for Cyberpunk 2077 2.50.
 * Patches the loaded executable's two atlas-size immediates, never the EXE file.
 * Native tagging logic and the request for 32 slots are unchanged. */
#include "atlas_patch.h"
#include "red4ext_api.h"
#include "win_api.h"
#define EXPORT __declspec(dllexport)
#define BUILD_TIMESTAMP 0x6ad55dafu
#define BUILD_IMAGE_SIZE 0x0581b000u
#define PATCH_RVA 0x0221352eu
#define PATCH_SIZE 28u
static uint8_t *patchSite;
static uint16_t Read16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1]<<8)); }
static uint32_t Read32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
static void SetSemVer(SemVer *v,uint8_t major,uint16_t minor,uint32_t patch) {
    v->major=major; v->minor=minor; v->patch=patch; v->prerelease.type=0; v->prerelease.number=0;
}
EXPORT uint32_t Supports(void) { return 1; }
EXPORT void Query(PluginInfo *info) {
    if(!info) return;
    SetSemVer(&info->sdk,1,0,0);
    info->name=L"Tag Marker Atlas Fix";
    info->author=L"Shaydera";
    SetSemVer(&info->version,0,1,0);
    info->runtime.major=3; info->runtime.minor=0; info->runtime.build=88; info->runtime.revision=46235;
}
static uint8_t *FindSite(uint8_t *image,Logger *log,PluginHandle handle) {
    if(!image || Read16(image)!=0x5a4d) return NULL;
    uint32_t pe=Read32(image+0x3c);
    if(pe>0x1000 || Read32(image+pe)!=0x00004550 || Read16(image+pe+4)!=0x8664) return NULL;
    if(Read32(image+pe+8)!=BUILD_TIMESTAMP || Read16(image+pe+24)!=0x20b || Read32(image+pe+24+56)!=BUILD_IMAGE_SIZE) {
        log->Error(handle,"Unsupported executable build: atlas patch was NOT applied.");
        return NULL;
    }
    uint16_t count=Read16(image+pe+6),optSize=Read16(image+pe+20);
    size_t headers=(size_t)pe+24+optSize;
    if(count>64 || optSize<112 || headers+(size_t)count*40>0x1000) return NULL;
    for(size_t i=0;i<count;++i) {
        uint8_t *section=image+headers+i*40;
        if(section[0]!='.' || section[1]!='t' || section[2]!='e' || section[3]!='x' || section[4]!='t' || section[5]!=0) continue;
        uint32_t size=Read32(section+8),rva=Read32(section+12);
        if(!(Read32(section+36)&0x20000000u) || rva>=BUILD_IMAGE_SIZE || size>BUILD_IMAGE_SIZE-rva) return NULL;
        size_t offset=0;
        int result=AtlasFind(image+rva,size,&offset);
        if(result!=ATLAS_OK || (size_t)rva+offset!=PATCH_RVA) {
            log->Error(handle,"Expected unique Stealth atlas signature was not found at the verified RVA: no patch applied.");
            return NULL;
        }
        return image+rva+offset;
    }
    return NULL;
}
static int ChangeSize(uint8_t *site,int restore) {
    uint32_t oldProtection=0,ignored=0;
    if(!VirtualProtect(site,PATCH_SIZE,PAGE_EXECUTE_READWRITE,&oldProtection)) return 0;
    int changed=AtlasRewrite(site,PATCH_SIZE,restore)==ATLAS_OK;
    if(!changed) {
        VirtualProtect(site,PATCH_SIZE,oldProtection,&ignored);
        return 0;
    }
    if(!FlushInstructionCache(GetCurrentProcess(),site,PATCH_SIZE)) {
        AtlasRewrite(site,PATCH_SIZE,!restore);
        FlushInstructionCache(GetCurrentProcess(),site,PATCH_SIZE);
        VirtualProtect(site,PATCH_SIZE,oldProtection,&ignored);
        return 0;
    }
    if(!VirtualProtect(site,PATCH_SIZE,oldProtection,&ignored)) {
        AtlasRewrite(site,PATCH_SIZE,!restore);
        FlushInstructionCache(GetCurrentProcess(),site,PATCH_SIZE);
        VirtualProtect(site,PATCH_SIZE,oldProtection,&ignored);
        return 0;
    }
    return 1;
}
EXPORT _Bool Main(PluginHandle handle,uint8_t reason,const Sdk *sdk) {
    if(!sdk || !sdk->logger) return 0;
    Logger *log=sdk->logger;
    if(reason==0) {
        if(patchSite) return 1;
        uint8_t *site=FindSite((uint8_t*)GetModuleHandleW(L"Cyberpunk2077.exe"),log,handle);
        if(!site) { log->Error(handle,"Tag Marker Atlas Fix patch inactive: executable validation failed."); return 0; }
        if(!ChangeSize(site,0)) { log->Error(handle,"Tag Marker Atlas Fix patch inactive: could not apply verified memory patch."); return 0; }
        patchSite=site;
        log->Info(handle,"Tag Marker Atlas Fix patch applied: Stealth 1024x1024 -> 2048x2048; requested slots remain 32.");
    } else if(reason==1 && patchSite) {
        if(ChangeSize(patchSite,1)) log->Info(handle,"Tag Marker Atlas Fix patch restored original bytes.");
        else log->Warn(handle,"Tag Marker Atlas Fix original bytes could not be restored; no foreign modifications were overwritten.");
        patchSite=NULL;
    }
    return 1;
}
/* No work in DllMain. RED4ext calls Main explicitly after loading the plugin. */
int DllMain(void *module,uint32_t reason,void *reserved) {
    (void)module; (void)reason; (void)reserved; return 1;
}
