@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ========================================
echo Running Pure Native Window Demo (No Vulkan)
echo ========================================
echo.

mvn -q compile exec:java -Dexec.mainClass="fastvulkan.DemoPureWindow"
