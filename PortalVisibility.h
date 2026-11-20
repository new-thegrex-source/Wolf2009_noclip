#pragma once
// Replace only the root portal flood. Preserve the original flow's downstream
// entity, interaction and shadow processing. RVAs verified against retail.
static bool portalPatched = false;
static unsigned char* RetailBase() { 
#ifdef FREECAM_PORTAL_TEST
return testImage;
#else
return reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
#endif
 }
static void __fastcall FloodEveryArea(unsigned char* world, void*, const float* camera, int, const unsigned char* stack) {
 auto base=RetailBase();
 auto areas=*reinterpret_cast<unsigned char**>(world+0x9c);
 const int count=*reinterpret_cast<int*>(world+0xa0);
 if(!areas||count<=0||count>65536)return;
 using Flood=void(__thiscall*)(void*,const float*,int,const unsigned char*);
 auto flood=reinterpret_cast<Flood>(base+0xd6fb0);
 for(int i=0;i<count;++i){
  auto area=areas+i*0x220;
  // Native root processing builds scissors, frusta, stamps and view lists.
  // Only its traversal into neighbouring areas is suppressed for this call.
  auto& links=*reinterpret_cast<void**>(area+0x164);
  auto savedLinks=links;links=nullptr;
  __try { flood(world,camera,i,stack); }
  __finally { links=savedLinks; }
 }
}
static bool SetPortalBypass(bool enable) {
    if (portalPatched == enable) return true;
    auto base = RetailBase();
    auto site = base + 0xd8298;
    // JL outside-area skip + argument setup + CALL recursive flood (19 bytes).
    const unsigned char original[] = {0x7c,0x11,0x8d,0x4c,0x24,0x10,0x51,0x50,0x8d,0x55,0x08,0x52,0x8b,0xce,0xe8,0x05,0xed,0xff,0xff};
    unsigned char patched[sizeof(original)];
    memcpy(patched, original, sizeof(original));
    patched[0] = patched[1] = 0x90; // Also build visibility when cameraArea == -1.
    const int relative = static_cast<int>(reinterpret_cast<unsigned char*>(&FloodEveryArea) - (site + sizeof(original)));
    memcpy(patched + 15, &relative, 4);
    if (memcmp(site, enable ? original : patched, sizeof(original))) {
        Log("Portal hook refused: instruction signature mismatch"); return false;
    }
    DWORD protection;
    if (!VirtualProtect(site, sizeof(original), PAGE_EXECUTE_READWRITE, &protection)) return false;
    memcpy(site, enable ? patched : original, sizeof(original));
    FlushInstructionCache(GetCurrentProcess(), site, sizeof(original));
    DWORD ignored;
    VirtualProtect(site, sizeof(original), protection, &ignored);
    portalPatched = enable;
    return true;
}


