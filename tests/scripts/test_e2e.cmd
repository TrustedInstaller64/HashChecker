@echo off
chcp 65001 >nul 2>&1
setlocal EnableDelayedExpansion

:: -- Switch to project root -------------------------------
cd /d "%~dp0..\.."

:: -- Find hashchecker.exe ---------------------------------
set "HASHCHECKER="
if exist "build\hashchecker.exe" (
    set "HASHCHECKER=build\hashchecker.exe"
) else if exist "build\Release\hashchecker.exe" (
    set "HASHCHECKER=build\Release\hashchecker.exe"
) else (
    echo ==============================================
    echo   ERROR: hashchecker.exe not found
    echo   Run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    echo        cmake --build build --parallel
    echo ==============================================
    pause
    exit /b 1
)

echo ==============================================
echo  HashChecker E2E Test
echo  Platform: Windows
echo  Binary: %HASHCHECKER%
echo ==============================================
echo.

:: -- Resolve TEMP long path ------------------------------
for %%A in ("%TEMP%") do set "TMPDIR=%%~fA\hc_e2e_%RANDOM%"
set "DIR_A=%TMPDIR%\dir_a"
set "DIR_B=%TMPDIR%\dir_b"
mkdir "%DIR_A%" 2>nul
mkdir "%DIR_B%" 2>nul

set PASS=0
set FAIL=0

echo Temp: %TMPDIR%
echo.

:: -- 1. Generate 1000 x 2KB random files in dir_a ---------
echo [1/4] Generating 1000 x 2KB random files...
powershell -NoProfile -Command "1..1000 | ForEach-Object {$b=New-Object byte[] 2048;(New-Object Random).NextBytes($b);[IO.File]::WriteAllBytes(($env:DIR_A+'\file_'+$_+'.bin'),$b)}"
echo   Done: 1000 files written to dir_a

:: -- 2. Copy dir_a to dir_b -------------------------------
echo [2/4] Copying dir_a to dir_b...
xcopy /E /I /Q "%DIR_A%" "%DIR_B%" >nul
echo   Done: dir_a and dir_b are identical

:: -- 3. Verify consistency (exit 0) -----------------------
echo [3/4] Verifying identical directories (expect exit 0)...

call :check "MD5"     0 md5
call :check "SHA1"    0 sha1
call :check "SHA256"  0 sha256

:: -- 4. Replace dir_b with different random files ----------
echo [4/4] Replacing dir_b with 1000 different random files...
rmdir /S /Q "%DIR_B%" 2>nul
mkdir "%DIR_B%" 2>nul
powershell -NoProfile -Command "1..1000 | ForEach-Object {$b=New-Object byte[] 2048;(New-Object Random).NextBytes($b);[IO.File]::WriteAllBytes(($env:DIR_B+'\file_'+$_+'.bin'),$b)}"
echo   Done: dir_b replaced with different content

:: -- 5. Verify differences (exit 1) -----------------------
echo.
echo [5/5] Verifying different directories (expect exit 1)...

call :check "MD5"     1 md5
call :check "SHA1"    1 sha1
call :check "SHA256"  1 sha256

:: -- 6. Check DIFFERENCES marker in output -----------------
echo.
echo [6/6] Checking DIFFERENCES in report output...
"%HASHCHECKER%" -t "%DIR_A%" -r "%DIR_B%" -a sha256 > "%TMPDIR%\report.txt" 2>&1
findstr /C:"DIFFERENCES" "%TMPDIR%\report.txt" >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo   PASS: DIFFERENCES marker found
    set /a PASS=PASS+1
) else (
    echo   FAIL: DIFFERENCES marker not found
    set /a FAIL=FAIL+1
)

:: -- Cleanup ----------------------------------------------
echo.
echo Cleaning up...
rmdir /S /Q "%TMPDIR%" 2>nul
echo   Removed: %TMPDIR%

:: -- Results ----------------------------------------------
echo.
echo ==============================================
set /a TOTAL=PASS+FAIL
echo  Result: %PASS%/%TOTAL% passed
if %FAIL% equ 0 (
    echo  Status: ALL PASSED
    echo ==============================================
    pause
    exit /b 0
) else (
    echo  Status: %FAIL% FAILED
    echo ==============================================
    pause
    exit /b 1
)

:: -- Helper: run hashchecker and check exit code -----------
:check
set "label=%~1"
set "expected=%~2"
set "algo=%~3"

echo   [%label%] expect exit %expected% ...

"%HASHCHECKER%" -t "%DIR_A%" -r "%DIR_B%" -a "%algo%" -q >nul 2>&1
if %ERRORLEVEL% equ %expected% (
    echo          PASS
    set /a PASS=PASS+1
) else (
    echo          FAIL ^(exit %ERRORLEVEL%, expected %expected%^)
    set /a FAIL=FAIL+1
)
exit /b
