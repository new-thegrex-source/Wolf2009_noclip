#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <cstdlib>
#include "RetailBuild.h"
static WNDPROC previousProc=nullptr;
static HWND gameWindow=nullptr;
static bool cullingOff=false,active=true;
struct Saved {unsigned char* object=nullptr;int integer=0;float real=0;};
static Saved saved[sizeof(kCvars)/sizeof(kCvars[0])];
static unsigned char* changedPlayer=nullptr;
static unsigned char originalNoclip=0;
static void Log(const char* text){char path[MAX_PATH];GetModuleFileNameA(nullptr,path,MAX_PATH);char* slash=strrchr(path,'\\');if(!slash)return;strcpy_s(slash+1,MAX_PATH-size_t(slash+1-path),"WolfFreecam.log");HANDLE f=CreateFileA(path,FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(f!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(f,text,DWORD(strlen(text)),&n,nullptr);WriteFile(f,"\r\n",2,&n,nullptr);CloseHandle(f);}}
#include "PortalVisibility.h"
static bool Build(HMODULE module,DWORD timestamp,DWORD size){__try{auto base=reinterpret_cast<unsigned char*>(module);if(!base)return false;auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);return dos->e_magic==IMAGE_DOS_SIGNATURE&&nt->Signature==IMAGE_NT_SIGNATURE&&nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386&&nt->FileHeader.TimeDateStamp==timestamp&&nt->OptionalHeader.SizeOfImage==size;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
static unsigned char* Cvar(size_t index){__try{auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));auto record=base+kCvars[index].rva;auto internal=*reinterpret_cast<unsigned char**>(record+0x2c);if(!internal||internal==record)return nullptr;const char* name=*reinterpret_cast<const char**>(internal+4);if(!name||strcmp(name,kCvars[index].name))return nullptr;return internal;}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}}
static void RestoreCvars(){SetPortalBypass(false);__try{for(size_t i=0;i<_countof(kCvars);++i){if(saved[i].object&&saved[i].object==Cvar(i)){*reinterpret_cast<int*>(saved[i].object+0x24)=saved[i].integer;*reinterpret_cast<float*>(saved[i].object+0x28)=saved[i].real;}saved[i].object=nullptr;}cullingOff=false;}__except(EXCEPTION_EXECUTE_HANDLER){Log("Cvar restore refused: runtime state changed");}}
static void ToggleCulling(){if(cullingOff){RestoreCvars();Log("Culling restored");return;}__try{for(size_t i=0;i<_countof(kCvars);++i)if(!Cvar(i)){Log("Culling unchanged: cvar object validation failed");return;}if(!SetPortalBypass(true)){Log("Culling unchanged: portal hook failed");return;}for(size_t i=0;i<_countof(kCvars);++i){auto p=Cvar(i);saved[i]={p,*reinterpret_cast<int*>(p+0x24),*reinterpret_cast<float*>(p+0x28)};*reinterpret_cast<int*>(p+0x24)=0;*reinterpret_cast<float*>(p+0x28)=0.f;}cullingOff=true;Log("Portal/area visibility bypass enabled; hardware occlusion, entity and distance culling disabled");}__except(EXCEPTION_EXECUTE_HANDLER){RestoreCvars();Log("Culling change aborted");}}
static unsigned char* Player(){__try{HMODULE module=GetModuleHandleW(L"Gamex86.dll");if(!Build(module,0x4ade51d0,0x970000))return nullptr;auto base=reinterpret_cast<unsigned char*>(module);const unsigned char expected[]={0x56,0x8b,0xf1,0x8b,0x86,0x4c,0x3d,0x06,0x00};if(memcmp(base+0x6dd00,expected,sizeof(expected)))return nullptr;using Getter=unsigned char*(__thiscall*)(void*);auto player=reinterpret_cast<Getter>(base+0x6dd00)(base+0x61f0b0);if(!player||*reinterpret_cast<void**>(player)!=base+0x5479e4||player[0x95b]>1)return nullptr;return player;}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}}
// GameLocal::Printf: retail variadic member uses cdecl with explicit this.
// Recovered at Gamex86 RVA 6C140; also used by ListPlayerStates.
static void PrintNoclip(bool enabled){
 const char* message=enabled?"Noclip ON":"Noclip Off";
 Log(message);
 __try {
  auto module=GetModuleHandleW(L"Gamex86.dll");
  if(!Build(module,0x4ade51d0,0x970000))return;
  auto base=reinterpret_cast<unsigned char*>(module);
  using Print=void(__cdecl*)(void*,const char*,...);
  reinterpret_cast<Print>(base+0x6c140)(base+0x61f0b0,"%s\n",message);
 }__except(EXCEPTION_EXECUTE_HANDLER){Log("Native console print failed");}
}
static bool portalModeKnown=false;
static bool requestedPortalBypass=false;
static bool ApplyPortalMode(bool bypass){
 auto record=RetailBase()+0x710688;
 auto cv=*reinterpret_cast<unsigned char**>(record+0x2c);
 if(!cv||cv==record||strcmp(*reinterpret_cast<const char**>(cv+4),"r_occlusionCullPortals"))return false;
 if(!SetPortalBypass(bypass))return false;
 *reinterpret_cast<int*>(cv+0x24)=bypass?0:1;
 *reinterpret_cast<float*>(cv+0x28)=bypass?0.f:1.f;
 return true;
}
static bool RequestPortalMode(bool bypass){
 if(!ApplyPortalMode(bypass))return false;
 requestedPortalBypass=bypass;portalModeKnown=true;
 Log(bypass?"Requested portal mode: 0 / bypass ON":"Requested portal mode: 1 / bypass OFF");
 return true;
}
static void ToggleNoclip(){__try{auto p=Player();if(!p){Log("Noclip unchanged: no validated local player; load a level first");return;}if(changedPlayer!=p){changedPlayer=p;originalNoclip=p[0x95b];}const bool next=p[0x95b]==0;if(!RequestPortalMode(next)){Log("Noclip unchanged: portal mode could not be applied");return;}p[0x95b]=next?1:0;PrintNoclip(next);}__except(EXCEPTION_EXECUTE_HANDLER){Log("Noclip change aborted: player changed");}}
static void __cdecl ConsoleNoclip(const void*) { if(active)ToggleNoclip(); }
static unsigned char* consoleCvar=nullptr;
static int consoleOldInt=0;
static float consoleOldFloat=0;
static void EnableNativeConsole(){
 if(consoleCvar)return;
 __try {
  auto record=RetailBase()+0x70c1e8;
  auto internal=*reinterpret_cast<unsigned char**>(record+0x2c);
  if(!internal||internal==record||strcmp(*reinterpret_cast<const char**>(internal+4),"com_allowConsole"))return;
  consoleOldInt=*reinterpret_cast<int*>(internal+0x24);
  consoleOldFloat=*reinterpret_cast<float*>(internal+0x28);
  *reinterpret_cast<int*>(internal+0x24)=1;
  *reinterpret_cast<float*>(internal+0x28)=1.f;
  consoleCvar=internal;Log("Native tilde console enabled: com_allowConsole=1");
 }__except(EXCEPTION_EXECUTE_HANDLER){Log("Console cvar not ready");}
}
static void SyncPortalSetting(){
 // A runtime cvar refresh must not silently reverse an explicit mod command.
 if(!portalModeKnown)return;
 __try { if(!ApplyPortalMode(requestedPortalBypass))Log("Requested portal mode could not be maintained"); }
 __except(EXCEPTION_EXECUTE_HANDLER){Log("Portal setting sync failed");}
}
static void __cdecl ConsolePortalSetting(const void* arguments){
 if(!active)return;
 __try {
  auto args=static_cast<const unsigned char*>(arguments);
  int argc=*reinterpret_cast<const int*>(args);
  if(argc!=2){Log("Usage: r_occlusionCullPortals 0|1");return;}
  const char* value=*reinterpret_cast<const char* const*>(args+8);
  if(!value||(strcmp(value,"0")&&strcmp(value,"1"))){Log("Expected portal value 0 or 1");return;}
  auto record=RetailBase()+0x710688;
  auto cv=*reinterpret_cast<unsigned char**>(record+0x2c);
  if(!cv||cv==record||strcmp(*reinterpret_cast<const char**>(cv+4),"r_occlusionCullPortals"))return;
  const bool disable=!strcmp(value,"0");
  if(!RequestPortalMode(disable))return;
  *reinterpret_cast<int*>(cv+0x24)=disable?0:1;
  *reinterpret_cast<float*>(cv+0x28)=disable?0.f:1.f;
  const char* message=disable?"Portal bypass ON (r_occlusionCullPortals 0)":"Portal bypass OFF (r_occlusionCullPortals 1)";
  Log(message);
  auto module=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"Gamex86.dll"));
  using Print=void(__cdecl*)(void*,const char*,...);
  reinterpret_cast<Print>(module+0x6c140)(module+0x61f0b0,"%s\n",message);
 }__except(EXCEPTION_EXECUTE_HANDLER){Log("Portal command failed argument validation");}
}
static void RegisterConsoleCommands(){
 static void* registered=nullptr;
 __try {
  auto module=GetModuleHandleW(L"Gamex86.dll");
  if(!Build(module,0x4ade51d0,0x970000))return;
  auto system=*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(module)+0x61f050);
  if(!system||system==registered)return;
  auto table=*reinterpret_cast<void***>(system);
  using Add=void(__thiscall*)(void*,const char*,void(__cdecl*)(const void*),int,const char*,void*);
  reinterpret_cast<Add>(table[3])(system,"noclip",ConsoleNoclip,0x10,"Toggle WolfFreecam noclip",nullptr);
  reinterpret_cast<Add>(table[3])(system,"r_occlusionCullPortals",ConsolePortalSetting,0x10,"0: bypass portal visibility; 1: restore",nullptr);registered=system;Log("Native console noclip command registered");
 }__except(EXCEPTION_EXECUTE_HANDLER){Log("Console registration failed");}
}
static bool nativeConsoleKeysBlocked=false;
static bool SetNativeConsoleKeysBlocked(bool block){
 if(nativeConsoleKeysBlocked==block)return true;
 auto branch=RetailBase()+0x48171;
 if(branch[0]!=(block?0x74:0xeb)||branch[1]!=0x67)return false;
 DWORD old;if(!VirtualProtect(branch,2,PAGE_EXECUTE_READWRITE,&old))return false;
 branch[0]=block?0xeb:0x74;FlushInstructionCache(GetCurrentProcess(),branch,2);
 DWORD ignored;VirtualProtect(branch,2,old,&ignored);nativeConsoleKeysBlocked=block;return true;
}
static void Restore(){SetNativeConsoleKeysBlocked(false);if(portalModeKnown)ApplyPortalMode(false);portalModeKnown=false;RestoreCvars();if(consoleCvar){*reinterpret_cast<int*>(consoleCvar+0x24)=consoleOldInt;*reinterpret_cast<float*>(consoleCvar+0x28)=consoleOldFloat;consoleCvar=nullptr;}__try{auto p=Player();if(p&&p==changedPlayer)p[0x95b]=originalNoclip;}__except(EXCEPTION_EXECUTE_HANDLER){}changedPlayer=nullptr;active=false;Log("Mod disabled; tracked values restored. Restart game to remove DLL.");}
static bool ToggleNativeConsole(){
 __try {
  auto base=RetailBase();auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
  auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
  auto sections=IMAGE_FIRST_SECTION(nt);unsigned char* object=nullptr;
  for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){
   if(!(sections[i].Characteristics&IMAGE_SCN_MEM_WRITE))continue;
   auto start=base+sections[i].VirtualAddress;
   for(DWORD j=0;j+4<=sections[i].Misc.VirtualSize;j+=4)
    if(*reinterpret_cast<void**>(start+j)==base+0x5dda94){if(object)return false;object=start+j;}
  }
  if(!object)return false;
  auto table=*reinterpret_cast<void***>(object);
  if(table[4]!=base+0x48160||table[5]!=base+0x40690||table[7]!=base+0x48d30||table[8]!=base+0x48e80)return false;
  using IsOpen=bool(__thiscall*)(void*);using Close=void(__thiscall*)(void*);using Open=void(__thiscall*)(void*,int);
  if(reinterpret_cast<IsOpen>(table[5])(object))reinterpret_cast<Close>(table[8])(object);
  else reinterpret_cast<Open>(table[7])(object,0);
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){Log("Direct console toggle failed");return false;}
}
static LRESULT CALLBACK WindowProc(HWND h,UINT message,WPARAM w,LPARAM l){if(active){EnableNativeConsole();RegisterConsoleCommands();SyncPortalSetting();if(cullingOff){auto current=Player();if(!current||!current[0x95b])RestoreCvars();}}if(active&&(message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&((l>>16)&0xff)==0x29){if(!(l&(1L<<30)))ToggleNativeConsole();return 0;}if(active&&(message==WM_KEYUP||message==WM_SYSKEYUP)&&((l>>16)&0xff)==0x29)return 0;if(active&&(message==WM_CHAR||message==WM_SYSCHAR)&&((l>>16)&0xff)==0x29)return 0;if(active&&message==WM_KEYDOWN&&!(l&(1L<<30))){if(w==VK_F8){Log("F8 bypass disabled: previous experimental portal patch was incorrect");return 0;}if(w==VK_F9){ToggleNoclip();return 0;}if(w==VK_F12){Restore();return 0;}}if(message==WM_NCDESTROY){Restore();gameWindow=nullptr;}return previousProc?CallWindowProcW(previousProc,h,message,w,l):DefWindowProcW(h,message,w,l);}
static BOOL CALLBACK FindWindowForProcess(HWND h,LPARAM){DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(h)&&!GetWindow(h,GW_OWNER)){RECT client={};GetClientRect(h,&client);if(GetModuleHandleW(L"d3d9.dll")&&GetModuleHandleW(L"Gamex86.dll")&&!GetWindow(h,GW_CHILD)&&client.right>=320&&client.bottom>=200){gameWindow=h;return FALSE;}}return TRUE;}
static DWORD WINAPI Install(void*){if(!Build(GetModuleHandleW(nullptr),0x562b029a,0x1a9a000)){Log("Unsupported Wolf2.exe build: mod inactive");return 0;}for(unsigned attempt=0;attempt<240&&!gameWindow;++attempt){EnumWindows(FindWindowForProcess,0);if(!gameWindow)Sleep(500);}if(!gameWindow){Log("Game window not found");return 0;}SetLastError(0);previousProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(gameWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(WindowProc)));if(!previousProc){Log("Window hook failed");return 0;}if(!SetNativeConsoleKeysBlocked(true))Log("Native console key suppression failed");Log("WolfFreecam v14 single-tilde loaded: noclip command / F9, F12 restore. Experimental F8 disabled. Changes run on window thread.");return 0;}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(module);HANDLE thread=CreateThread(nullptr,0,Install,nullptr,0,nullptr);if(thread)CloseHandle(thread);}return TRUE;}













