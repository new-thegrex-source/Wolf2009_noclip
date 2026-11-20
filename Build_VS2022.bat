@echo off
setlocal
where cmake >nul 2>nul
if not errorlevel 1 (
  set "CMAKE_EXE=cmake"
) else (
  set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
"%CMAKE_EXE%" -S "%~dp0" -B "%~dp0VS2022" -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 exit /b 1
"%CMAKE_EXE%" --build "%~dp0VS2022" --config Release
exit /b %errorlevel%
