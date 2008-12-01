@echo off
rem
rem Eis um pequeno script tosco para compilar no windows sem precisar fazer
rem muita coisa. Requer que o dev-cpp esteja instalado em C:\Dev-cpp\
rem

set PATH=%PATH%;C:\Dev-cpp\bin\

if not exist C:\Dev-cpp\bin\ goto sumiu

make -f Makefile.win

goto ok

:sumiu
echo Aonde vc instalou o dev-cpp? Nao encontrei-o em C:\Dev-cpp\
goto fim

:ok
echo pronto, veja se toscal.exe e tokenize.exe apareceram

:fim
