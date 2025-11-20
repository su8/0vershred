:TOP
@CLS
@ECHO OFF
TITLE 0vershred
:BEGIN
if not exist C:\MingW\bin\0vershred.exe (
COPY 0vershred.exe C:\MingW\bin\0vershredr.exe
)
CLS
:loop
echo Provide some files to overwrite them in a row
SET /P NAME=
C:\MingW\bin\0vershred.exe %name%
PAUSE
GOTO loop