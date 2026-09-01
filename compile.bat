@echo off
:: FastVulkan Native DLL Compiler Script
:: Auto-detects Visual Studio, Vulkan SDK and JAVA_HOME

echo ========================================
echo FastVulkan Native Library Builder
echo ========================================

set LIB_NAME=fastvulkan

:: Try to find VS using vswhere.exe
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if not defined VS_PATH (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
    )
)

if not defined VS_PATH (
    echo ERROR: Visual Studio not found!
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%

:: Try to detect JAVA_HOME if not set
if not defined JAVA_HOME (
    if exist "C:\Program Files\Java\jdk-21.0.12.1" (
        set "JAVA_HOME=C:\Program Files\Java\jdk-21.0.12.1"
    ) else if exist "C:\Program Files\Java\jdk-25" (
        set "JAVA_HOME=C:\Program Files\Java\jdk-25"
    ) else if exist "C:\Program Files\Eclipse Adoptium\jdk-17-hotspot" (
        set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-17-hotspot"
    ) else if exist "C:\Program Files\Java\jdk-17" (
        set "JAVA_HOME=C:\Program Files\Java\jdk-17"
    )
)

if not defined JAVA_HOME (
    echo ERROR: JAVA_HOME not set!
    exit /b 1
)

echo Using JAVA_HOME: %JAVA_HOME%

:: Check Vulkan SDK
if not defined VULKAN_SDK (
    if exist "C:\VulkanSDK" (
        for /f "tokens=*" %%d in ('dir /b /ad /o-n "C:\VulkanSDK"') do (
            set "VULKAN_SDK=C:\VulkanSDK\%%d"
            goto :vulkan_found
        )
    )
)
:vulkan_found
if defined VULKAN_SDK (
    echo Using VULKAN_SDK: %VULKAN_SDK%
) else (
    echo WARNING: VULKAN_SDK environment variable not set. Assuming system vulkan-1.lib.
)

:: Setup environment
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

:: Create build directories
if not exist release mkdir release
if not exist build mkdir build

set VK_INC=
set VK_LIB=
if defined VULKAN_SDK (
    set VK_INC=/I "%VULKAN_SDK%\Include"
    set VK_LIB=/LIBPATH:"%VULKAN_SDK%\Lib"
)

:: Compile C++ source
cl.exe /O2 /W3 /std:c++17 /MD /EHsc /LD ^
   /I "%JAVA_HOME%\include" ^
   /I "%JAVA_HOME%\include\win32" ^
   /I "native\include" ^
   %VK_INC% ^
   /Fo:build\ ^
   /Fe:release\%LIB_NAME%.dll ^
   native\src\*.cpp ^
   vulkan-1.lib user32.lib gdi32.lib shcore.lib dwmapi.lib uxtheme.lib ^
   /link /DLL /MACHINE:X64 %VK_LIB%

if %ERRORLEVEL% == 0 (
    echo.
    echo [SUCCESS] DLL built at: release\%LIB_NAME%.dll
    copy release\%LIB_NAME%.dll . >nul 2>&1
    if exist sign-natives.bat call sign-natives.bat
) else (
    echo.
    echo [FAILED] Compilation failed.
    exit /b 1
)

echo.
