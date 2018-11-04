@echo off
pushd build
cl -Zi ../distshift.c
REM -GS- -Gs9999999 -link -STACK:0x100000,0x100000
rm *.obj
popd