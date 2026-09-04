@echo off
chcp 65001 >nul
cd /d "%~dp0"

set MAVEN_OPTS=--enable-native-access=ALL-UNNAMED
if exist "C:\Users\andre\tools\apache-maven-3.9.9\bin" set PATH=C:\Users\andre\tools\apache-maven-3.9.9\bin;%PATH%
if defined VULKAN_SDK set PATH=%VULKAN_SDK%\Bin;%PATH%
if exist "C:\Program Files\VulkanSDK\1.4.357.0\Bin" set PATH=C:\Program Files\VulkanSDK\1.4.357.0\Bin;%PATH%

echo ========================================================
echo  FastVulkan Showcase Demos Launcher
echo ========================================================
echo  [1] DemoScreenshotMeshWarp  (240x135 Grid 65k Mesh 14-Algorithm Warp + [V] Dots)
echo  [2] DemoGraphics2DCompare   (1:1 Pixel Calibration Suite)
echo ========================================================
set /p DEMO_CHOICE="Waehle Demo [1-2] (Standard: 1): "

if "%DEMO_CHOICE%"=="" set DEMO_CHOICE=1
if "%DEMO_CHOICE%"=="1" set MAIN_CLASS=fastvulkan.DemoScreenshotMeshWarp
if "%DEMO_CHOICE%"=="2" set MAIN_CLASS=fastvulkan.DemoGraphics2DCompare

echo.
echo Starting %MAIN_CLASS%...
cd examples\Demo
call mvn compile exec:java -Dexec.mainClass="%MAIN_CLASS%" -q
cd ..\..
pause
