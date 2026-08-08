@echo off
chcp 65001 >nul
title Build MuOnline Main.exe (Release x86)

set MSBUILD="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
set SLN="G:\Novooooo\SRCMainGS Projeto\Source\novo12345\Main.sln"
set OUTDIR="C:\MuOnline\MuOnline season 6\MuOnline Season 6\Client_2\"
set EXENAME=Main.exe

echo ============================================================
echo  BUILD MUONLINE MAIN.EXE - RELEASE x86
echo ============================================================
echo Solution: %SLN%
echo Output:   %OUTDIR%%EXENAME%
echo.

if not exist %MSBUILD% (
    echo ERRO: MSBuild nao encontrado em %MSBUILD%
    echo Verifique se o Visual Studio 2022 esta instalado.
    pause
    exit /b 1
)

echo [1/3] Limpando build anterior...
%MSBUILD% %SLN% /p:Configuration=Release /p:Platform=x86 /t:Clean /v:quiet

echo [2/3] Compilando (Rebuild)...
%MSBUILD% %SLN% /p:Configuration=Release /p:Platform=x86 /t:Rebuild /v:minimal
if errorlevel 1 (
    echo.
    echo ============================================================
    echo ERRO NA COMPILACAO!
    echo ============================================================
    pause
    exit /b 1
)

echo [3/3] Copiando para pasta raiz do cliente...
if not exist %OUTDIR% mkdir %OUTDIR%
copy /Y "C:\MuOnline\MuOnline season 6\MuOnline Season 6\MuOnline Season 6\Client_2\%EXENAME%" %OUTDIR%%EXENAME% >nul
if errorlevel 1 (
    echo ERRO: Falha ao copiar %EXENAME% para %OUTDIR%
    pause
    exit /b 1
)

echo.
echo ============================================================
echo BUILD CONCLUIDO COM SUCESSO!
echo ============================================================
echo Executavel: %OUTDIR%%EXENAME%
echo.
dir %OUTDIR%%EXENAME%
echo.
pause