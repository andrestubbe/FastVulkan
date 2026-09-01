@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================
echo Running FastVulkan Window Live-Resize Demo
echo ========================================

:: Build native dll if needed
call compile.bat

if not exist release\fastvulkan.dll (
    echo Error: fastvulkan.dll build failed.
    exit /b 1
)

:: Compile and run Java demo quietly
mvn -q compile exec:java
