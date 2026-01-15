@echo off
@echo %date% %time% - SUSPENDED >> test.log
C:\Opt\edifier.exe --power-off --no-confirm
@echo %date% %time% - EXECUTED >> test.log
