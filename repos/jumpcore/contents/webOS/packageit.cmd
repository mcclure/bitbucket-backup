set STAGING_DIR=STAGING\com.andimcclure.jumpcoredemo

rmdir /s /Y %STAGING_DIR%
del *.ipk
mkdir %STAGING_DIR%
call buildit.cmd
copy .\appinfo.json %STAGING_DIR%
copy .\icon.png %STAGING_DIR%
xcopy /E /Y .\Jumpcore\* %STAGING_DIR%
echo filemode.755=jumpcore.exe> %STAGING_DIR%\package.properties
palm-package %STAGING_DIR%
