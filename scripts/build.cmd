@echo off
setlocal
cd /d "%~dp0.."
if not exist deps\pdfium\include\fpdfview.h (
  echo Falta PDFium. Ejecutar scripts\setup-pdfium.ps1 primero.
  exit /b 1
)
where cl.exe >nul 2>nul
if errorlevel 1 (
  if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
  ) else (
    echo Ejecutar desde Developer Command Prompt x64 de Visual Studio.
    exit /b 1
  )
)
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include src\main.cpp /Fo:build\main.obj /Fe:build\Chucao.exe /link /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /DELAYLOAD:pdfium.dll /LIBPATH:deps\pdfium\lib pdfium.dll.lib delayimp.lib user32.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib
if errorlevel 1 exit /b 1
copy /y deps\pdfium\bin\pdfium.dll build\pdfium.dll >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /I deps\pdfium\include tools\pdf-smoke.cpp /Fo:build\pdf-smoke.obj /Fe:build\pdf-smoke.exe /link /LIBPATH:deps\pdfium\lib pdfium.dll.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /utf-8 /DNOMINMAX tools\layout-test.cpp /Fo:build\layout-test.obj /Fe:build\layout-test.exe
if errorlevel 1 exit /b 1
echo Compilacion completada: build\Chucao.exe
