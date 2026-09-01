@echo off
chcp 65001 >nul
cd /d "%~dp0"

set MAVEN_OPTS=--enable-native-access=ALL-UNNAMED
if exist "C:\Users\andre\tools\apache-maven-3.9.9\bin" set PATH=C:\Users\andre\tools\apache-maven-3.9.9\bin;%PATH%
if defined VULKAN_SDK set PATH=%VULKAN_SDK%\Bin;%PATH%
if exist "C:\Program Files\VulkanSDK\1.4.357.0\Bin" set PATH=C:\Program Files\VulkanSDK\1.4.357.0\Bin;%PATH%

echo ========================================================
echo  FastVulkan vs Java2D 1:1 Pixel Calibration Suite
echo ========================================================

if not exist release\fastvulkan.dll (
    echo Building native fastvulkan.dll...
    call compile.bat
)
if not exist release\fastvulkan.dll (
    echo Error: DLL build failed.
    pause
    exit /b 1
)

if not exist "src\main\resources\native" mkdir "src\main\resources\native"
copy /Y "release\fastvulkan.dll" "src\main\resources\native\fastvulkan.dll" >nul

echo Building Core Library...
call mvn clean install -DskipTests -q
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    pause
    exit /b %ERRORLEVEL%
)

set FASTCORE_DIR=%USERPROFILE%\.fastcore\native\fastvulkan
if not exist "%FASTCORE_DIR%" mkdir "%FASTCORE_DIR%"
copy /Y "release\fastvulkan.dll" "%FASTCORE_DIR%\fastvulkan.dll" >nul
powershell -NoProfile -ExecutionPolicy Bypass -Command "Unblock-File '%FASTCORE_DIR%\fastvulkan.dll'" >nul

echo Running Compare Demo...
cd examples\Demo
call mvn compile exec:java -Dexec.mainClass="fastvulkan.DemoGraphics2DCompare" -q
if %ERRORLEVEL% NEQ 0 (
    echo Demo failed.
    pause
    exit /b 1
)

cd ..\..
pause
