@echo off
set LB_PROJECT_REPO=angleproject
call %~dp0/wdk/setup %~n0 %*
setlocal
pushd %LB_PROJECT_ROOT%

set NAME=%LB_PROJECT_NAME%
cd src\%NAME%

(for /f "usebackq tokens=3 delims=>=/ " %%i in (`findstr ClCompile %NAME%.vcxproj`) do if /I "%%~xi"==".cpp" echo %%i) > sources.tmp

cl %LB_CL_OPTS% -EHsc -Fe%NAME%.dll -LD -DANGLE_DISABLE_TRACE=1 -DWIN32=1 -DLIBEGL_EXPORTS=1 -DNDEBUG=1 -DNOMINMAX -D_SECURE_SCL=0 -FI_70_new -I.. -I..\..\include -I"%~dp0include" -I"%DXSDK_DIR%\Include" -DNOMINMAX @sources.tmp "%~dp0dwmapi_%LB_TARGET_ARCH%.lib" /link"%LB_LINK_OPTS% gdi32.lib user32.lib opengl32.lib d3d9.lib dxguid.lib %~dp0/../../bin/%LB_TARGET_OS%/%LB_TARGET_ARCH%/libGLESv2.lib /DEF:%NAME%.def /DELAYLOAD:dwmapi.dll delayimp.lib "

call %~dp0/wdk/install %LB_PROJECT_NAME%.dll
call %~dp0/wdk/install %LB_PROJECT_NAME%.lib
call %~dp0/wdk/install %LB_PROJECT_NAME%.pdb
endlocal
popd
