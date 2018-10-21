@echo off
pushd build
cl -Zi ../distshift.c
rm *.obj
popd