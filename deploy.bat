@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

:: ==================== 配置参数 ====================

set SOURCE_DIR=build\Desktop_Qt_6_11_0_MSVC2022_64bit-Release
set APP_NAME=simple-video-player.exe
set WINDEPLOYQT=C:\Qt6\6.11.0\msvc2022_64\bin\windeployqt6.exe
set DEPLOY_DIR=deploy

echo ========================================
echo 开始部署 %APP_NAME%
echo ========================================
echo.

:: 1. 检查源文件是否存在
set SOURCE_FILE=%SOURCE_DIR%\%APP_NAME%
if not exist "%SOURCE_FILE%" (
    echo [错误] 找不到源文件: %SOURCE_FILE%
    pause
    exit /b 1
)

:: 2. 清空并创建部署目录
if exist "%DEPLOY_DIR%" (
    echo [2/5] 清空部署目录...
    rmdir /s /q "%DEPLOY_DIR%" 2>nul
)
mkdir "%DEPLOY_DIR%"

:: 3. 复制可执行文件到部署目录
copy /Y "%SOURCE_FILE%" "%DEPLOY_DIR%\" > nul
if errorlevel 1 (
    echo [错误] 复制文件失败
    pause
    exit /b 1
)

:: 4. 检查 windeployqt 是否存在
if not exist "%WINDEPLOYQT%" (
    echo [错误] 找不到 windeployqt6.exe: %WINDEPLOYQT%
    echo.
    pause
    exit /b 1
)

:: 5. 运行 windeployqt 部署依赖
echo [5/5] 正在部署 Qt 依赖库...
cd /d "%DEPLOY_DIR%"
"%WINDEPLOYQT%" %APP_NAME%
if errorlevel 1 (
    echo [错误] windeployqt 执行失败
    cd /d "%~dp0"
    pause
    exit /b 1
)
cd /d "%~dp0"

echo.
echo ========================================
echo 部署完成
echo ========================================
echo.

:: pause