@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Running FastVulkan Window Live-Resize Demo
echo ========================================

:: Build native dll if needed
if not exist release\fastvulkan.dll (
    echo Building native fastvulkan.dll...
    call compile.bat
)

if not exist release\fastvulkan.dll (
    echo Error: fastvulkan.dll build failed.
    exit /b 1
)

:: Compile and run Java demo
mvn clean compile exec:java -Dexec.mainClass="fastvulkan.DemoWindow"
