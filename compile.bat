@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ========================================================
echo  Compiling FastVulkan Native DLL (Vulkan 1.3)
echo ========================================================

:: 1. Detect JAVA_HOME
if "%JAVA_HOME%"=="" (
    for %%d in (
        "C:\Program Files\Java\jdk-21.0.12.1"
        "C:\Program Files\Java\jdk-21"
        "C:\Program Files\Java\jdk-17"
        "C:\Program Files\Java\jdk-25"
    ) do (
        if exist %%d (
            set "JAVA_HOME=%%~d"
            goto :found_java
        )
    )
)
:found_java
if "%JAVA_HOME%"=="" (
    echo [ERROR] JAVA_HOME not found.
    exit /b 1
)
echo Using JAVA_HOME: %JAVA_HOME%

:: 2. Detect Vulkan SDK
if not defined VULKAN_SDK (
    if exist "C:\VulkanSDK" (
        for /f "tokens=*" %%d in ('dir /b /ad /o-n "C:\VulkanSDK"') do (
            set "VULKAN_SDK=C:\VulkanSDK\%%d"
            goto :found_vk
        )
    )
    if exist "C:\Program Files\VulkanSDK\1.4.357.0" (
        set "VULKAN_SDK=C:\Program Files\VulkanSDK\1.4.357.0"
        goto :found_vk
    )
)
:found_vk
if not defined VULKAN_SDK (
    echo [ERROR] VULKAN_SDK not found.
    exit /b 1
)
echo Using VULKAN_SDK: %VULKAN_SDK%

:: 3. Detect Visual Studio vcvars64.bat
where cl.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    set "VS_DIR="
    for %%v in (
        "C:\Program Files\Microsoft Visual Studio\18\Community"
        "C:\Program Files\Microsoft Visual Studio\2026\Community"
        "C:\Program Files\Microsoft Visual Studio\2022\Community"
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
    ) do (
        if exist "%%~v\VC\Auxiliary\Build\vcvars64.bat" (
            set "VS_DIR=%%~v"
            goto :found_vs
        )
    )
    :found_vs
    if not defined VS_DIR (
        echo [ERROR] Visual Studio compiler not found.
        exit /b 1
    )
    echo Found Visual Studio at: !VS_DIR!
    call "!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat" >nul
)

if not exist "target\classes" mkdir "target\classes"
if not exist "src\main\resources" mkdir "src\main\resources"

glslc -fshader-stage=vert native\src\shader_2d.vert -o native\src\shader_vert.spv
glslc -fshader-stage=frag native\src\shader_2d.frag -o native\src\shader_frag.spv
copy /y native\src\shader_vert.spv src\main\resources\shaders\
copy /y native\src\shader_frag.spv src\main\resources\shaders\
copy /y native\src\shader_vert.spv target\classes\shaders\
copy /y native\src\shader_frag.spv target\classes\shaders\
copy /y native\src\shader_vert.spv .
copy /y native\src\shader_frag.spv .

cl.exe /O2 /W3 /std:c++17 /MD /EHsc /LD ^
   /I "%JAVA_HOME%\include" ^
   /I "%JAVA_HOME%\include\win32" ^
   /I "%VULKAN_SDK%\Include" ^
   /Fe:target\classes\FastVulkan.dll ^
   native\VulkanBackend.cpp ^
   /link /DLL /MACHINE:X64 /LIBPATH:"%VULKAN_SDK%\Lib" vulkan-1.lib user32.lib gdi32.lib

if %ERRORLEVEL% EQU 0 (
    copy /y target\classes\FastVulkan.dll . >nul 2>&1
    copy /y target\classes\FastVulkan.dll src\main\resources\FastVulkan.dll >nul 2>&1
    echo.
    echo ========================================================
    echo [SUCCESS] FastVulkan.dll compiled successfully
    echo ========================================================
) else (
    echo.
    echo [ERROR] Native compilation failed.
    exit /b 1
)
endlocal
