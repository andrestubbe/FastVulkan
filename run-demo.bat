@echo off
chcp 65001 >nul
cd /d %~dp0

set MAVEN_OPTS=--enable-native-access=ALL-UNNAMED
if exist C:\Users\andre\tools\apache-maven-3.9.9\bin set PATH=C:\Users\andre\tools\apache-maven-3.9.9\bin;%PATH%
if defined VULKAN_SDK set PATH=%VULKAN_SDK%\Bin;%PATH%
if exist C:\Program Files\VulkanSDK\1.4.357.0\Bin set PATH=C:\Program Files\VulkanSDK\1.4.357.0\Bin;%PATH%

echo ========================================================
echo  FastVulkan — Screenshot Mesh Warp Demo (65k Mesh)
echo ========================================================

cd examples\Demo
call mvn compile exec:java -Dexec.mainClass=fastvulkan.DemoScreenshotMeshWarp -q

cd ..\..
pause
