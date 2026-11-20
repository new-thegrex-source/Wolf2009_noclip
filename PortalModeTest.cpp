#define FREECAM_PORTAL_TEST
static unsigned char* testImage;
#include "WolfFreecam.cpp"
#include <cstdio>
static void Check(bool ok){if(!ok){puts("FAIL");exit(1);}}
int main(){
 testImage=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xa00000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(testImage!=nullptr);
 unsigned char cv[0x40]={};*reinterpret_cast<const char**>(cv+4)="r_occlusionCullPortals";
 *reinterpret_cast<unsigned char**>(testImage+0x710688+0x2c)=cv;
 const unsigned char original[]={0x7c,0x11,0x8d,0x4c,0x24,0x10,0x51,0x50,0x8d,0x55,0x08,0x52,0x8b,0xce,0xe8,0x05,0xed,0xff,0xff};
 memcpy(testImage+0xd8298,original,sizeof(original));
 Check(RequestPortalMode(true));Check(portalPatched&&*reinterpret_cast<int*>(cv+0x24)==0);
 for(int i=0;i<100;++i){*reinterpret_cast<int*>(cv+0x24)=1;*reinterpret_cast<float*>(cv+0x28)=1;SyncPortalSetting();Check(portalPatched&&*reinterpret_cast<int*>(cv+0x24)==0&&*reinterpret_cast<float*>(cv+0x28)==0);}
 Check(RequestPortalMode(false));Check(!portalPatched&&*reinterpret_cast<int*>(cv+0x24)==1);Check(!memcmp(testImage+0xd8298,original,sizeof(original)));
 *reinterpret_cast<int*>(cv+0x24)=0;SyncPortalSetting();Check(!portalPatched&&*reinterpret_cast<int*>(cv+0x24)==1);
 puts("PASS: ON=0, 100 external resets resisted, OFF=1, original code restored");
 VirtualFree(testImage,0,MEM_RELEASE);
}
