@echo off
setlocal enabledelayedexpansion

:: 상위 폴더 내 examples 디렉토리 생성
if not exist "..\examples" (
    mkdir "..\examples"
)

echo ========================================
echo [Current Directory Folder List]
echo ========================================

:: 1. 폴더 리스트업
set count=0
for /d %%D in (*) do (
    set /a count+=1
    set "folder[!count!]=%%D"
    echo [!count!] %%D
)

if %count%==0 (
    echo No folders found.
    pause
    exit /b
)

echo ========================================
:: 2. 복사할 폴더 선택
set /p choice="Select folder number to copy: "

if not defined folder[%choice%] (
    echo [Error] Invalid selection.
    pause
    exit /b
)
set "selected_folder=!folder[%choice%]!"

:: 3. 새 폴더명 입력
set "new_name="
set /p new_name="Enter new folder name (Press Enter to keep '!selected_folder!'): "

if "!new_name!"=="" (
    set "new_name=!selected_folder!"
)

:: 4. 복사 수행
set "target_path=..\examples\!new_name!"

echo.
echo Copying '!selected_folder!' to '!target_path!'...
xcopy "!selected_folder!" "!target_path!" /E /I /H /K /Y

:: 5. CMakeLists.txt 존재 여부 확인 후 서브루틴 호출
set "cmake_file=!target_path!\CMakeLists.txt"
if exist "!cmake_file!" (
    :: 괄호 충돌을 피하기 위해 외부 서브루틴으로 점프
    call :UpdateCMake "!cmake_file!" "!new_name!"
) else (
    echo.
    echo [Warning] CMakeLists.txt not found. Skipping modification.
)

echo.
echo [System] All tasks completed successfully.
pause
exit /b

:: ========================================
:: Subroutine: CMakeLists.txt 업데이트 로직 (UTF-8 No BOM)
:: ========================================
:UpdateCMake
echo.
echo [System] Updating CMakeLists.txt with project name '%~2'...

:: 독립된 공간이므로 괄호() 사용에 제약이 없음
powershell -NoProfile -Command "$utf8NoBom = New-Object System.Text.UTF8Encoding $false; $content = (Get-Content -Path '%~1' -Raw -Encoding UTF8) -replace 'project\(default_project\)', 'project(%~2)'; [System.IO.File]::WriteAllText('%~1', $content, $utf8NoBom)"

echo [System] CMakeLists.txt update completed.
exit /b