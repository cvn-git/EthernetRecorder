set SDK_PATH=C:\ti\mcu_plus_sdk_am243x_08_05_00_24
set FLASH_PATH=%SDK_PATH%\tools\boot

rmdir /S /Q dist

pyinstaller -F %FLASH_PATH%\uart_uniflash_gui.py

set PREBUILT_PATH=%FLASH_PATH%\sbl_prebuilt\am243x-lp
copy %PREBUILT_PATH%\sbl_uart_uniflash.release.tiimage .\dist\
copy %PREBUILT_PATH%\sbl_ospi.release.tiimage .\dist\
copy %PREBUILT_PATH%\sbl_null.release.tiimage .\dist\
