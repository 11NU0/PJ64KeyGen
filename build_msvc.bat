@echo off
REM Builds the console (PJ64KeyGen.exe) and GUI (PJ64KeyGenGui.exe) keygens with
REM MSVC, linking the zlib copy that ships with project64-develop
REM (Source\3rdParty\zlib) so no external dependency is needed.

setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARS%" set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARS%" (
  echo Could not find vcvarsall.bat - edit VCVARS in this script.
  exit /b 1
)

set ZLIB=%~dp0..\project64-develop\Source\3rdParty\zlib
if not exist "%ZLIB%\zlib.h" (
  echo Could not find zlib sources at "%ZLIB%".
  exit /b 1
)

call "%VCVARS%" x64 >nul

pushd "%~dp0"
if exist build\ rmdir /s /q build
mkdir build

set CXXFLAGS=/nologo /std:c++17 /EHsc /O2 /W3 /MT /Fo:build\ /I"%ZLIB%"
set ZLIBSRC="%ZLIB%\adler32.c" "%ZLIB%\compress.c" "%ZLIB%\crc32.c" "%ZLIB%\deflate.c" ^
  "%ZLIB%\infback.c" "%ZLIB%\inffast.c" "%ZLIB%\inflate.c" "%ZLIB%\inftrees.c" ^
  "%ZLIB%\trees.c" "%ZLIB%\uncompr.c" "%ZLIB%\zutil.c"
set LIBS=advapi32.lib
set GUILIBS=%LIBS% user32.lib comdlg32.lib comctl32.lib shell32.lib

cl %CXXFLAGS% /Fe:build\PJ64KeyGen.exe PJ64KeyGen.cpp %ZLIBSRC% %LIBS%
if errorlevel 1 goto :failed

rc /nologo /fo build\PJ64KeyGenGui.res PJ64KeyGenGui.rc
if errorlevel 1 goto :failed

cl %CXXFLAGS% /Fe:build\PJ64KeyGenGui.exe PJ64KeyGenGui.cpp build\PJ64KeyGenGui.res %ZLIBSRC% %GUILIBS%
if errorlevel 1 goto :failed

echo.
echo Built build\PJ64KeyGen.exe and build\PJ64KeyGenGui.exe
popd
endlocal
exit /b 0

:failed
echo Build failed.
popd
endlocal
exit /b 1