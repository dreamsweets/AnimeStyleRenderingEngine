cd..

ECHO CURPATH=%cd%
xcopy AnimeStyleRendering\ThirdParty\Libs\Debug\*.dll Bin\ /s /d /y
xcopy AnimeStyleRendering\ThirdParty\Libs\Debug\*.exp Bin\ /s /d /y

xcopy AnimeStyleRendering\Shaders\*.fx Bin\Shaders\ /s /d /y

pause