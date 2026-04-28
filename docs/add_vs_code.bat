@echo off
echo ==========================================
echo [System] VS Code Context Menu Registration
echo ==========================================

:: VS Code가 설치된 절대 경로 정의 (User Installer 기준)
set "VSCODE_PATH=%LocalAppData%\Programs\Microsoft VS Code\Code.exe"

:: 만약 시스템 전체 설치(System Installer) 버전이라면 아래 주석(::)을 해제하고 위 줄을 지우세요.
:: set "VSCODE_PATH=C:\Program Files\Microsoft VS Code\Code.exe"

if not exist "%VSCODE_PATH%" (
    echo [Error] VS Code executable not found at: %VSCODE_PATH%
    echo Please check the installation path.
    pause
    exit /b
)

echo.
echo Injecting registry keys...

:: 1. 파일 우클릭 시 메뉴 추가
reg add "HKCR\*\shell\VSCode" /ve /t REG_SZ /d "Code(으)로 열기" /f >nul
reg add "HKCR\*\shell\VSCode" /v "Icon" /t REG_SZ /d "\"%VSCODE_PATH%\"" /f >nul
reg add "HKCR\*\shell\VSCode\command" /ve /t REG_SZ /d "\"%VSCODE_PATH%\" \"%%1\"" /f >nul

:: 2. 폴더 아이콘 우클릭 시 메뉴 추가
reg add "HKCR\Directory\shell\VSCode" /ve /t REG_SZ /d "Code(으)로 열기" /f >nul
reg add "HKCR\Directory\shell\VSCode" /v "Icon" /t REG_SZ /d "\"%VSCODE_PATH%\"" /f >nul
reg add "HKCR\Directory\shell\VSCode\command" /ve /t REG_SZ /d "\"%VSCODE_PATH%\" \"%%1\"" /f >nul

:: 3. 폴더 내부 빈 공간 우클릭 시 메뉴 추가
reg add "HKCR\Directory\Background\shell\VSCode" /ve /t REG_SZ /d "Code(으)로 열기" /f >nul
reg add "HKCR\Directory\Background\shell\VSCode" /v "Icon" /t REG_SZ /d "\"%VSCODE_PATH%\"" /f >nul
reg add "HKCR\Directory\Background\shell\VSCode\command" /ve /t REG_SZ /d "\"%VSCODE_PATH%\" \"%%V\"" /f >nul

echo.
echo [System] Registry injection completed successfully.
pause