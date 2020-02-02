@echo off

plink -P 10022 root@localhost -pw "" "killall -9 gdbserver jumpcore.exe"
plink -P 10022 root@localhost -pw "" "rm -rf /media/internal/Jumpcore"
plink -P 10022 root@localhost -pw "" "mkdir /media/internal/Jumpcore"
pscp -scp -P 10022 -pw "" -r Jumpcore/* root@localhost:/media/internal/Jumpcore
