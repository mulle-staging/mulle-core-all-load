@echo off
setlocal enabledelayedexpansion

REM Get repo URL and reference from arguments
set "REPO_URL=%~1"
set "REPO_REF=%~2"
if "%REPO_URL%"=="" set "REPO_URL=https://github.com/mulle-core/mulle-core-all-load.git"

REM Create temp directory
for /f %%i in ('powershell -command "[System.IO.Path]::GetTempPath() + [System.Guid]::NewGuid().ToString()"') do set "TEMP_DIR=%%i"
mkdir "%TEMP_DIR%"
cd /d "%TEMP_DIR%"

echo Creating test project in %TEMP_DIR%
echo Using repository: %REPO_URL%
if not "%REPO_REF%"=="" echo Using reference: %REPO_REF%

REM Initialize git repo
git init
git config user.email "test@example.com"
git config user.name "Test User"
git config protocol.file.allow always

REM Add mulle-core-all-load as submodule
git submodule add "%REPO_URL%" mulle-core-all-load
if not "%REPO_REF%"=="" (
    cd mulle-core-all-load
    git checkout "%REPO_REF%"
    cd ..
)
git submodule update --init --recursive

REM Create CMakeLists.txt
(
 echo cmake_minimum_required^(VERSION 3.15^)
 echo project^(submodule-test^)
 echo add_subdirectory^(mulle-core-all-load/mulle-core^)
 echo add_subdirectory^(mulle-core-all-load^)
 echo add_executable^(test main.c^)
 echo target_link_libraries^(test PRIVATE mulle-core-all-load^)
) > CMakeLists.txt

REM Create main.c
(
echo #include ^<mulle-core-all-load/mulle-core-all-load.h^>
echo.
echo int main^(^) {
echo     // TODO: test something
echo     mulle_fprintf^(stdout, "mulle-core-all-load submodule test: %%s\n", "SUCCESS"^);
echo.    
echo     return 0;
echo }
) > main.c

REM Build and test
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
build\Release\test.exe

echo Test completed successfully
cd /d %~dp0
rmdir /s /q "%TEMP_DIR%"
