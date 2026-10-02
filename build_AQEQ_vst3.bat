@echo off
setlocal EnableExtensions
title AQEQ - Compilar VST3
cd /d "%~dp0"

echo ==================================================
echo   AQEQ - Compilador de VST3  [JUCE + CMake]
echo ==================================================
echo.

rem ---------- 1. Permisos de administrador ----------
rem COPY_PLUGIN_AFTER_BUILD copia a C:\Program Files\Common Files\VST3,
rem y eso necesita admin.
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Pidiendo permisos de administrador...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

rem ---------- 2. Ordenar archivos en Source\ ----------
if not exist "Source" mkdir "Source"
for %%F in (PluginProcessor.cpp PluginProcessor.h PluginEditor.cpp PluginEditor.h) do (
    if exist "%%F" if not exist "Source\%%F" move /Y "%%F" "Source\%%F" >nul
)
if not exist "Source\PluginProcessor.cpp" goto :err_source
if not exist "Source\PluginEditor.cpp" goto :err_source
if not exist "CMakeLists.txt" goto :err_cmakelists

rem ---------- 3. CMake ----------
where cmake >nul 2>&1
if %errorlevel% neq 0 if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "PATH=%ProgramFiles%\CMake\bin;%PATH%"
where cmake >nul 2>&1
if %errorlevel% neq 0 goto :err_cmake

rem ---------- 4. Buscar JUCE ----------
if defined JUCE_DIR goto :check_juce
if exist "%~dp0juce_path.txt" set /p JUCE_DIR=<"%~dp0juce_path.txt"
if defined JUCE_DIR goto :check_juce

for %%D in ("C:\JUCE" "D:\JUCE" "C:\dev\JUCE" "%USERPROFILE%\JUCE" "%USERPROFILE%\Documents\JUCE" "%USERPROFILE%\Downloads\JUCE" "%~dp0JUCE" "%~dp0..\JUCE") do (
    if exist "%%~D\CMakeLists.txt" if exist "%%~D\modules" set "JUCE_DIR=%%~D"
)
if defined JUCE_DIR goto :check_juce

:ask_juce
echo.
echo No encontre JUCE automaticamente.
echo Pega la ruta de la carpeta de JUCE, la que contiene CMakeLists.txt y la carpeta modules.
set "JUCE_DIR="
set /p "JUCE_DIR=Ruta de JUCE: "
if not defined JUCE_DIR goto :ask_juce

:check_juce
set "JUCE_DIR=%JUCE_DIR:"=%"
if not exist "%JUCE_DIR%\CMakeLists.txt" goto :bad_juce
if not exist "%JUCE_DIR%\modules" goto :bad_juce
>"%~dp0juce_path.txt" echo %JUCE_DIR%
set "JUCE_CMAKE=%JUCE_DIR:\=/%"
echo JUCE: %JUCE_DIR%
echo.

rem ---------- 5. Configurar y compilar ----------
if exist "build\CMakeCache.txt" findstr /C:"NMake Makefiles" "build\CMakeCache.txt" >nul 2>&1 && rmdir /s /q "build"
echo [1/2] Configurando proyecto...
cmake -S . -B build -DJUCE_DIR="%JUCE_CMAKE%"
if %errorlevel% neq 0 goto :fail_cfg

echo.
echo [2/2] Compilando Release...
cmake --build build --config Release --parallel
if %errorlevel% neq 0 goto :fail

rem ---------- 6. Copia local del VST3 ----------
set "FOUND="
for /d /r "build" %%P in (AQEQ.vst3) do if exist "%%P" set "FOUND=%%P"

set "OUTDIR=%~dp0AQEQ_VST3"
if defined FOUND (
    if exist "%OUTDIR%" rmdir /s /q "%OUTDIR%"
    xcopy "%FOUND%" "%OUTDIR%\AQEQ.vst3" /E /I /Y /Q >nul
)

echo.
echo ==================================================
echo   LISTO
echo ==================================================
if exist "%CommonProgramFiles%\VST3\AQEQ.vst3" echo Instalado en: %CommonProgramFiles%\VST3\AQEQ.vst3
if exist "%OUTDIR%\AQEQ.vst3" echo Copia local:  %OUTDIR%\AQEQ.vst3
echo.
echo En FL Studio: Options ^> Manage plugins ^> Find installed plugins.
echo.
pause
exit /b 0

:bad_juce
echo.
echo [ERROR] "%JUCE_DIR%" no parece una carpeta de JUCE.
echo Tiene que tener CMakeLists.txt y la carpeta modules adentro.
set "JUCE_DIR="
if exist "%~dp0juce_path.txt" del "%~dp0juce_path.txt"
goto :ask_juce

:err_source
echo [ERROR] No encuentro PluginProcessor.cpp / PluginEditor.cpp.
echo Deja los 4 archivos Plugin*.cpp/.h junto a este .bat o dentro de la carpeta Source.
goto :end_err

:err_cmakelists
echo [ERROR] Falta CMakeLists.txt junto a este .bat.
goto :end_err

:err_cmake
echo [ERROR] No encuentro CMake. Instalalo desde https://cmake.org/download/
echo y marca la opcion "Add CMake to the system PATH".
goto :end_err

:fail_cfg
if exist "build" rmdir /s /q "build"
:fail
echo.
echo [ERROR] Fallo la compilacion. Revisa los mensajes de arriba.
echo Si dice que no encuentra un compilador, instala Visual Studio con
echo "Desktop development with C++".

:end_err
echo.
pause
exit /b 1
