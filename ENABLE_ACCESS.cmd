@echo off
:: GAYClient Internal 0.3.5-mouse-test Access Helper
:: Grants RX access to the DLL for all packaged applications (AppContainer)
:: Backup DACL is created automatically by icacls /save (if supported) or manually.

set DLL_PATH=%~dp0GAYClientInternal.dll
set BACKUP_FILE=%~dp0DACL_BACKUP.txt

echo Backing up current DACL...
icacls "%DLL_PATH%" /save "%BACKUP_FILE%" /T /C || echo Backup failed (may require admin)

echo Granting RX to S-1-15-2-1 (ALL_APP_PACKAGES)...
icacls "%DLL_PATH%" /grant *S-1-15-2-1:(RX) /T /C || echo Grant failed (may require admin)

echo Done. Run RESTORE_ACCESS.cmd to revert.
pause
