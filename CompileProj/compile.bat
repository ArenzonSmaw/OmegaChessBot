@echo off
if "%1"=="" (
    echo Usage: compile ^<source_file^>
    exit /b 1
)

del /q output\*.*
echo Compiling...
Debug\CompileProj.exe %1
if errorlevel 1 (
    echo Compilation failed.
	if exist output\semantic_error.txt (
        echo Semantic error:
        type output\semantic_error.txt
    ) else if exist output\parser_error.txt (
        echo Parser error:
        type output\parser_error.txt
    ) else if exist output\lexer_error.txt (
        echo Lexer error:
        type output\lexer_error.txt
    ) else (
        echo No error file found.
    )
    exit /b 1
)

echo Assembling...
dosbox -quiet -c "mount c C:\Users\arenz\source\repos\OmegaChessBot\CompileProj" -c "mount d C:\Tasm1.4\Tasm" -c "d:\tasm.exe c:\output\target.asm c:\output\target.obj" -c "exit"
if errorlevel 1 (
    echo Assembly failed.
    exit /b 1
)

echo Linking...
dosbox -quiet -c "mount c C:\Users\arenz\source\repos\OmegaChessBot\CompileProj" -c "mount d C:\Tasm1.4\Tasm" -c "d:\tlink.exe c:\output\target.obj" -c "exit"
if errorlevel 1 (
    echo Linking failed.
    exit /b 1
)

echo Running...

dosbox -c "mount c C:\Users\arenz\source\repos\OmegaChessBot\CompileProj" -c "c:" -c "echo off" -c "cls" -c "output\target.exe" -c "echo." -c "echo." -c "echo ALT+ENTER for fullscreen" -c "pause" -c "exit" 