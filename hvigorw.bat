@echo off
setlocal

if exist .\node_modules\.bin\hvigor.cmd (
  .\node_modules\.bin\hvigor.cmd %*
  exit /b %errorlevel%
)

where hvigor >nul 2>nul
if %errorlevel%==0 (
  hvigor %*
  exit /b %errorlevel%
)

echo hvigor not found.
echo Open this project in DevEco Studio first, or install hvigor in your environment, then rerun hvigorw.bat.
exit /b 127
