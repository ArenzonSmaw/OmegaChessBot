@echo off

if exist output\target.exe (
	echo RUNNING...
	dosbox -c "mount c C:\Users\arenz\source\repos\OmegaChessBot\CompileProj" -c "c:" -c "echo off" -c "cls" -c "output\target.exe" -c "echo." -c "echo ALT+ENTER for fullscreen" -c "pause" -c "exit"
) else (
	echo ERROR: no executable program found
)
