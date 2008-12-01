@echo off
rem Um script tosco para iniciar um shell do cmd.exe ja com as variaveis de
rem ambiente devidamente configuradas.
rem
rem Antes de usar substitua o conteudo da variavel HGUSER com nome e email
rem seus. Tome cuidado em preservar as aspas em torno de tudo e manter o
rem formato Nome <email@host>.
rem

if "%1"=="dont-go-further" goto proceed

:usual
start cmd.exe /k %0 dont-go-further

:proceed
set "HGUSER=Foolano Foolanovich <foolano@foolanovich.net>"

set "PATH=%PATH%;C:\Arquivos de programas\Mercurial\"
set "PATH=%PATH%;C:\Program Files\Mercurial\"

set VISUAL=notepad

echo Pronto.
echo Nao se esqueca:
echo
echo Para pegar:
echo   hg pull
echo   hg update
echo
echo Para enviar: 
echo   hg commit
echo   hg push
echo
