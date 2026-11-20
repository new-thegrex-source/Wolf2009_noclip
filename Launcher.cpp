#define wmain InjectorMain
#include "Injector.cpp"
#undef wmain
#include "LauncherConsole.h"
static DWORD WINAPI RunLauncher(void* parameter) {
 const auto arguments=static_cast<std::wstring*>(parameter); PostLog("Starting Wolf2.exe...\r\n");
 wchar_t path[MAX_PATH];GetModuleFileNameW(nullptr,path,MAX_PATH);
 std::wstring directory(path);directory.resize(directory.find_last_of(L"\\/"));
 std::wstring gameDirectory=directory;
 if(GetFileAttributesW((gameDirectory+L"\\Wolf2.exe").c_str())==INVALID_FILE_ATTRIBUTES)
  gameDirectory.resize(gameDirectory.find_last_of(L"\\/"));
 const auto exe=gameDirectory+L"\\Wolf2.exe";
 if(GetFileAttributesW(exe.c_str())==INVALID_FILE_ATTRIBUTES){MessageBoxW(nullptr,L"Wolf2.exe not found beside launcher or in parent folder.",L"Wolf2Enhaced",MB_ICONERROR);return 1;}
 if(GetFileAttributesW((directory+L"\\WolfFreecam.dll").c_str())==INVALID_FILE_ATTRIBUTES){MessageBoxW(nullptr,L"WolfFreecam.dll missing beside launcher.",L"Wolf2Enhaced",MB_ICONERROR);return 2;}
 std::wstring command=L"\""+exe+L"\" +set logfile 2 "+*arguments;
 STARTUPINFOW startup={sizeof(startup)};PROCESS_INFORMATION process={};
 if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,gameDirectory.c_str(),&startup,&process)){MessageBoxW(nullptr,L"Cannot launch Wolf2.exe.",L"Wolf2Enhaced",MB_ICONERROR);return 3;}
 CloseHandle(process.hThread);
 HANDLE watcher=nullptr;
 if(DuplicateHandle(GetCurrentProcess(),process.hProcess,GetCurrentProcess(),&watcher,SYNCHRONIZE,FALSE,0)){
  if(!PostMessageW(logWindow,WM_APP+2,0,reinterpret_cast<LPARAM>(watcher)))CloseHandle(watcher);
 }else PostLog("Cannot monitor game exit; console will stay open.\r\n");
 WaitForInputIdle(process.hProcess,30000);
 if(WaitForSingleObject(process.hProcess,0)==WAIT_OBJECT_0){CloseHandle(process.hProcess);return 4;}
 wchar_t* argv[]={path};std::ostringstream output;auto oldOut=std::cout.rdbuf(output.rdbuf());auto oldErr=std::cerr.rdbuf(output.rdbuf());int result=InjectorMain(1,argv);std::cout.rdbuf(oldOut);std::cerr.rdbuf(oldErr);PostLog(output.str());PostLog("Injection result: "+std::to_string(result)+"\r\n");CloseHandle(process.hProcess);
 if(result)MessageBoxW(nullptr,L"Game started, but DLL injection failed. See WolfFreecam.log or run WolfFreecamInjector.exe for diagnostics.",L"Wolf2Enhaced",MB_ICONERROR);
 return result;
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR arguments,int){
 if(!CreateLogWindow(instance))return 1;
 wchar_t path[MAX_PATH];GetModuleFileNameW(nullptr,path,MAX_PATH);std::wstring root(path);root.resize(root.find_last_of(L"\\/"));
 if(GetFileAttributesW((root+L"\\Wolf2.exe").c_str())==INVALID_FILE_ATTRIBUTES)root.resize(root.find_last_of(L"\\/"));
 tails.push_back({root+L"\\WolfFreecam.log"});tails.push_back({root+L"\\base\\qconsole.log"});
 wchar_t local[MAX_PATH];if(GetEnvironmentVariableW(L"LOCALAPPDATA",local,MAX_PATH))tails.push_back({std::wstring(local)+L"\\id Software\\WolfSP\\base\\qconsole.log"});
 for(auto& t:tails){WIN32_FILE_ATTRIBUTE_DATA data;if(GetFileAttributesExW(t.path.c_str(),GetFileExInfoStandard,&data))t.offset=(LONGLONG(data.nFileSizeHigh)<<32)|data.nFileSizeLow;}
 AppendLog("Wolf2Enhanced launcher console\r\nDLL log: WolfFreecam.log\r\nGame log: waiting for qconsole.log (logfile 2)\r\n");
 std::wstring command(arguments);launchWorker=CreateThread(nullptr,0,RunLauncher,&command,0,nullptr);
 if(!launchWorker)AppendLog("Failed to start launcher worker\r\n");
 MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 if(launchWorker)CloseHandle(launchWorker);DeleteObject(logFont);return 0;
}

