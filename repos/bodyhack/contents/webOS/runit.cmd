@echo off
call buildit.cmd
call uploadit.cmd
plink -P 10022 root@localhost -pw "" "/media/internal/Jumpcore/jumpcore.exe"

