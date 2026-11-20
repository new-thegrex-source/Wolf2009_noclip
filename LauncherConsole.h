/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
// Standalone adaptation of Doom3 neo/sys/win32/win_syscon.cpp window layout.
// Engine command input is omitted; this window displays launcher and file logs.
#include <vector>
#include <sstream>
static HWND logWindow,logEdit,logStatus;
static HBRUSH logBrush=CreateSolidBrush(RGB(42,31,25)),statusBrush=CreateSolidBrush(RGB(128,128,128));
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
static HFONT logFont;
static HANDLE launchWorker;
static HANDLE watchedGame=nullptr;
static ULONGLONG closeDeadline=0;
struct TailFile {std::wstring path;LONGLONG offset=0;};
static std::vector<TailFile> tails;
static void AppendLog(const std::string& text){
 int n=MultiByteToWideChar(CP_ACP,0,text.data(),int(text.size()),nullptr,0);
 std::wstring w(n,0);MultiByteToWideChar(CP_ACP,0,text.data(),int(text.size()),w.data(),n);
 if(GetWindowTextLengthW(logEdit)>500000)SetWindowTextW(logEdit,L"");
 SendMessageW(logEdit,EM_SETSEL,WPARAM(-1),LPARAM(-1));SendMessageW(logEdit,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(w.c_str()));
}
static void PollLogs(){for(auto& t:tails){
 HANDLE f=CreateFileW(t.path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
 if(f==INVALID_HANDLE_VALUE)continue;LARGE_INTEGER size; if(GetFileSizeEx(f,&size)){
 if(size.QuadPart<t.offset)t.offset=0;LARGE_INTEGER pos;pos.QuadPart=t.offset;SetFilePointerEx(f,pos,nullptr,FILE_BEGIN);
 char buffer[8193];DWORD read=0;unsigned batches=0;while(batches++<8&&ReadFile(f,buffer,8192,&read,nullptr)&&read){AppendLog(std::string(buffer,read));t.offset+=read;}}
 CloseHandle(f);
}}
static LRESULT CALLBACK LogProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){case WM_SIZE:{int width=LOWORD(l),height=HIWORD(l);MoveWindow(logStatus,6,5,width-12,29,TRUE);MoveWindow(logEdit,6,40,width-30,height-96,TRUE);MoveWindow(GetDlgItem(h,1),6,height-24,70,22,TRUE);MoveWindow(GetDlgItem(h,2),83,height-24,70,22,TRUE);MoveWindow(GetDlgItem(h,3),width-78,height-24,70,22,TRUE);return 0;}
 case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{HDC dc=reinterpret_cast<HDC>(w);HWND control=reinterpret_cast<HWND>(l);if(control==logEdit){SetTextColor(dc,RGB(255,192,0));SetBkColor(dc,RGB(42,31,25));return reinterpret_cast<LRESULT>(logBrush);}if(control==logStatus){SetTextColor(dc,RGB(255,0,0));SetBkColor(dc,RGB(128,128,128));return reinterpret_cast<LRESULT>(statusBrush);}break;}
 case WM_TIMER:
  PollLogs();
  if(watchedGame&&WaitForSingleObject(watchedGame,0)==WAIT_OBJECT_0){
   CloseHandle(watchedGame);watchedGame=nullptr;
   closeDeadline=GetTickCount64()+3000;
   SetWindowTextW(logStatus,L"Console will close in 3 secounds");
   AppendLog("Console will close in 3 secounds\r\n");
  }
  if(closeDeadline&&GetTickCount64()>=closeDeadline&&(!launchWorker||WaitForSingleObject(launchWorker,0)!=WAIT_TIMEOUT))DestroyWindow(h);
  return 0;
 case WM_APP+2:if(watchedGame)CloseHandle(watchedGame);watchedGame=reinterpret_cast<HANDLE>(l);return 0;
 case WM_APP+1:{auto text=reinterpret_cast<std::string*>(l);AppendLog(*text);if(!closeDeadline)SetWindowTextA(logStatus,text->substr(0,text->find_first_of("\r\n")).c_str());delete text;return 0;}
 case WM_COMMAND:if(LOWORD(w)==1){SendMessageW(logEdit,EM_SETSEL,0,-1);SendMessageW(logEdit,WM_COPY,0,0);}if(LOWORD(w)==2)SetWindowTextW(logEdit,L"");if(LOWORD(w)==3)SendMessageW(h,WM_CLOSE,0,0);return 0;
 case WM_CLOSE:if(launchWorker&&WaitForSingleObject(launchWorker,0)==WAIT_TIMEOUT){ShowWindow(h,SW_MINIMIZE);return 0;}DestroyWindow(h);return 0;
 case WM_DESTROY:KillTimer(h,1);if(watchedGame){CloseHandle(watchedGame);watchedGame=nullptr;}PostQuitMessage(0);return 0;}
 return DefWindowProcW(h,m,w,l);
}
static void PostLog(const std::string& text){auto p=new std::string(text);if(!PostMessageW(logWindow,WM_APP+1,0,reinterpret_cast<LPARAM>(p)))delete p;}
static bool CreateLogWindow(HINSTANCE instance){
 WNDCLASSW wc={};wc.lpfnWndProc=LogProc;wc.hInstance=instance;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);wc.lpszClassName=L"WolfEnhancedWinConsole";RegisterClassW(&wc);
 logWindow=CreateWindowW(wc.lpszClassName,L"Wolf2Enhanced — Console",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,542,482,nullptr,nullptr,instance,nullptr);if(!logWindow)return false;
 logFont=CreateFontW(-11,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH,L"Courier New");
 logStatus=CreateWindowW(L"STATIC",L"Starting Wolf2Enhanced...",WS_CHILD|WS_VISIBLE,6,5,526,29,logWindow,nullptr,instance,nullptr);SendMessageW(logStatus,WM_SETFONT,reinterpret_cast<WPARAM>(logFont),TRUE);
 logEdit=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,6,40,690,380,logWindow,nullptr,instance,nullptr);SendMessageW(logEdit,WM_SETFONT,reinterpret_cast<WPARAM>(logFont),TRUE);SendMessageW(logEdit,EM_SETLIMITTEXT,600000,0);
 const wchar_t* names[]={L"copy",L"clear",L"quit"};for(int i=0;i<3;++i)CreateWindowW(L"BUTTON",names[i],WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,6+i*80,8,74,24,logWindow,reinterpret_cast<HMENU>(INT_PTR(i+1)),instance,nullptr);
 RECT client;GetClientRect(logWindow,&client);SendMessageW(logWindow,WM_SIZE,0,MAKELPARAM(client.right,client.bottom));SetTimer(logWindow,1,250,nullptr);ShowWindow(logWindow,SW_SHOW);UpdateWindow(logWindow);return true;
}




