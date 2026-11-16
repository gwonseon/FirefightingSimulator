@echo off
setlocal
chcp 65001 >nul

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0SetupProject.ps1"
set "SetupExitCode=%ERRORLEVEL%"

echo.
if not "%SetupExitCode%"=="0" (
    echo Project setup failed. Check the SetupLogs folder for details.
) else (
    echo Project setup completed successfully.
)
echo.
pause
exit /b %SetupExitCode%
