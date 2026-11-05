@echo off
:: GAYClient Internal 0.3.5-mouse-test Restore Access Helper
:: Restores DACL from backup

set DLL_PATH=%~dp0GAYClientInternal.dll
set BACKUP_FILE=%~dp0DACL_BACKUP.txt

if exist "%BACKUP_FILE%" (
    echo Restoring DACL from backup...
    icacls "%DLL_PATH%" /restore "%BACKUP_FILE%" /T /C || echo Restore failed
) else (
    echo Backup file not found. Cannot restore.
)

pause
