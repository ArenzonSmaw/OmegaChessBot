@echo off
cd c:\users\arenz\source\repos\omegachessbot\compileproj

echo lexer error example
pause
compile lexer_error.txt

echo parser error example
pause
compile parser_error.txt

echo semantic error example
pause
compile semantic_error.txt

echo input example 1: x=12, y=5, is x*y > 50?
pause
compile inputexample1.txt

echo input example 2: loop with i = 0, 2, 4; a = i+1, b = 1+2 on function ((3a+5b)%b)%10
pause
compile inputexample2.txt

echo basic: average calculation
pause
compile basic.txt

echo factorial and 15 boom: print factorials 1->5, then from 1->20 if %3: fizz, if %5: buss, if %15: fizzbuzz
pause
compile claudetest.txt

echo triangle print by inputted size
pause
compile ima.txt