@echo off

IF [%1] == [] (
	echo Enter build mode: d or r
	goto :eof
)

IF %1 == d (
	echo Debug Build
	set MODE=-Od -DDEBUG_MODE
)
IF %1 == r (
	echo Release Build
	SET MODE=-O2
)

rem set LIBS=user32.lib gdi32.lib 
rem set APP_TYPE=-subsystem:windows
rem set INCLUDES=/I
rem set EXPOTED_FUNCS=-EXPORT:UpdateAndRender

set COMPILER_FLAGS=%MODE% -MTd -WL -D_CRT_SECURE_NO_WARNINGS=1 -nologo -fp:fast -fp:except- -Gm- -EHsc -Zo -Oi -W4 -wd4100 -wd4458 -wd4505 -wd4201 -FC -Zi -GS-
set LINKER_FLAGS=%APP_TYPE% -incremental:no -opt:ref %LIBS%
set EXE_NAME=main

if not exist .\build mkdir .\build
pushd .\build

del *.pdb > NUL 2> NUL

rem cl  %COMPILER_FLAGS% ..\code\hy3d_engine.cpp -Fmhy3d_engine.map -LD -link -incremental:no -opt:ref -PDB:hy3d_engine_%RANDOM%.pdb %EXPOTED_FUNCS%

cl  %COMPILER_FLAGS% ..\code\main.cpp  -Fmain.map -Fe%EXE_NAME% -link %LINKER_FLAGS%
popd

:eof