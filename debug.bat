@echo off
echo Starting Debugger

pushd build
if exist main.sln (
	devenv main.sln
) else (
	devenv main.exe
)
popd