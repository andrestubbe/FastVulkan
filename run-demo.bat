@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Running FastVulkan Demo
echo ========================================

:: Build native dll if not exists
if not exist release\fastvulkan.dll (
    echo Building native dll first...
    call compile.bat
)

:: Compile and run Java demo
mvn compile exec:java -Dexec.mainClass="fastvulkan.Demo"
