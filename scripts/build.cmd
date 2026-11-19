@echo off
setlocal
cd /d "%~dp0.."
if not exist deps\pdfium\include\fpdfview.h (
  echo Falta PDFium. Ejecutar scripts\setup-pdfium.ps1 primero.
  exit /b 1
)
where cl.exe >nul 2>nul
if errorlevel 1 (
  call :FindCompiler
  if errorlevel 1 exit /b 1
)
if /i "%VSCMD_ARG_TGT_ARCH%"=="x86" (
  echo Se requiere el compilador x64. Usar Developer Command Prompt x64.
  exit /b 1
)
if not exist build mkdir build
rc /nologo /I src /I . /fo build\chucao.res src\chucao.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include src\main.cpp /Fo:build\main.obj /Fe:build\Chucao.exe /link build\chucao.res /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /DELAYLOAD:pdfium.dll /LIBPATH:deps\pdfium\lib pdfium.dll.lib delayimp.lib user32.lib gdi32.lib msimg32.lib comdlg32.lib comctl32.lib uxtheme.lib shell32.lib ole32.lib oleacc.lib oleaut32.lib uuid.lib dwmapi.lib gdiplus.lib advapi32.lib
if errorlevel 1 exit /b 1
copy /y deps\pdfium\bin\pdfium.dll build\pdfium.dll >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\pdf-smoke.cpp /Fo:build\pdf-smoke.obj /Fe:build\pdf-smoke.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DNOMINMAX tools\layout-test.cpp /Fo:build\layout-test.obj /Fe:build\layout-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\outline-test.cpp /Fo:build\outline-test.obj /Fe:build\outline-test.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\links-test.cpp /Fo:build\links-test.obj /Fe:build\links-test.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\password-test.cpp /Fo:build\password-test.obj /Fe:build\password-test.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX tools\reading-state-test.cpp /Fo:build\reading-state-test.obj /Fe:build\reading-state-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX tools\recent-test.cpp /Fo:build\recent-test.obj /Fe:build\recent-test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\messages-test.cpp /Fo:build\messages-test.obj /Fe:build\messages-test.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\robustness-test.cpp /Fo:build\robustness-test.obj /Fe:build\robustness-test.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX tools\association-test.cpp /Fo:build\association-test.obj /Fe:build\association-test.exe /link advapi32.lib shell32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX tools\preferences-test.cpp /Fo:build\preferences-test.obj /Fe:build\preferences-test.exe
if errorlevel 1 exit /b 1
echo Compilacion completada: build\Chucao.exe
exit /b 0

:FindCompiler
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto LegacyCompiler
set "VSROOT="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT goto LegacyCompiler
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
where cl.exe >nul 2>nul
exit /b %errorlevel%

:LegacyCompiler
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
  if errorlevel 1 exit /b 1
  where cl.exe >nul 2>nul
  exit /b
)
echo Faltan las herramientas de C++ x64 de Visual Studio y el Windows SDK.
echo Instalarlas desde Visual Studio Installer o usar Developer Command Prompt x64.
exit /b 1
