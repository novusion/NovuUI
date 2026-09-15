@echo off
rem ===========================================================================
rem NovuUI Component Gallery launcher for Windows.
rem
rem   run.bat                  build if needed, then open the gallery
rem   run.bat badge            open it on a component, by its registry id
rem   run.bat -c badge -p novutrade
rem                            any playground argument, passed through
rem
rem   run.bat build            build only
rem   run.bat tests            build, then run the test suites
rem   run.bat list             print the component registry and exit
rem   run.bat themes           print the theme profiles and exit
rem   run.bat clean            delete the build tree
rem
rem The build tree is reused, so only the first run is slow.
rem
rem Two things it deliberately does not guess:
rem
rem   Qt      more than one version is usually installed. Once the build tree
rem           exists, the Qt it was configured with is read back out of the
rem           CMake cache, so the DLLs on PATH are the ones the binary was
rem           actually linked against. Only a fresh configure picks a version,
rem           and NOVUUI_QT overrides even that.
rem   CMake   taken from the Qt install, then from PATH. NOVUUI_CMAKE overrides.
rem
rem NOVUUI_NO_PAUSE=1 stops it waiting for a keypress after a failure.
rem
rem Scripting this: use build, tests, list or themes. Those exit on their own.
rem A redirected bare "run.bat" waits for the gallery window to close, because
rem the gallery inherits the redirected handle - see the note further down.
rem ===========================================================================

setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "BUILD_DIR=build-gallery"
set "CONFIG=Release"
set "CACHE=%BUILD_DIR%\CMakeCache.txt"
set "RC=0"

rem --- what was asked for ----------------------------------------------------
set "MODE=run"
set "ARGS="
if /i "%~1"=="build"  set "MODE=build"  & shift & goto :modeset
if /i "%~1"=="tests"  set "MODE=tests"  & shift & goto :modeset
if /i "%~1"=="test"   set "MODE=tests"  & shift & goto :modeset
if /i "%~1"=="clean"  set "MODE=clean"  & shift & goto :modeset
if /i "%~1"=="list"   set "ARGS=--list"     & shift & goto :modeset
if /i "%~1"=="themes" set "ARGS=--profiles" & shift & goto :modeset

rem A bare word is a component id, so "run.bat badge" does the obvious thing.
if not "%~1"=="" (
    set "FIRST=%~1"
    if not "!FIRST:~0,1!"=="-" (
        set "ARGS=-c %~1"
        shift
    )
)
:modeset

rem Anything left over goes to the playground untouched.
:collect
if not "%~1"=="" (
    set "ARGS=!ARGS! %1"
    shift
    goto :collect
)

if "%MODE%"=="clean" goto :clean

rem --- cmake -----------------------------------------------------------------
if not defined NOVUUI_CMAKE (
    if exist "C:\Qt\Tools\CMake_64\bin\cmake.exe" (
        set "NOVUUI_CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
    ) else (
        for /f "delims=" %%C in ('where cmake.exe 2^>nul') do (
            if not defined NOVUUI_CMAKE set "NOVUUI_CMAKE=%%C"
        )
    )
)
if not defined NOVUUI_CMAKE (
    echo [error] CMake was not found. Install it with Qt, put it on PATH, or set
    echo         NOVUUI_CMAKE to its full path.
    set "RC=1"
    goto :end
)

rem --- Qt --------------------------------------------------------------------
rem An existing build tree decides for itself: reconfiguring it against a
rem different Qt than it was built with is how a gallery ends up loading one
rem version's DLLs into another version's binary.
if exist "%CACHE%" (
    for /f "tokens=2 delims==" %%V in ('findstr /b /c:"Qt6_DIR:PATH=" "%CACHE%"') do (
        set "QT_CMAKE_DIR=%%V"
    )
    if defined QT_CMAKE_DIR (
        set "QT_DIR=!QT_CMAKE_DIR:/lib/cmake/Qt6=!"
        set "QT_DIR=!QT_DIR:/=\!"
    )
)

if not defined QT_DIR (
    if defined NOVUUI_QT (
        set "QT_DIR=%NOVUUI_QT%"
    ) else (
        rem Newest wins: the loop overwrites and the listing is in version order.
        for /d %%D in ("C:\Qt\6.*") do (
            if exist "%%D\msvc2022_64\bin\Qt6Core.dll" set "QT_DIR=%%D\msvc2022_64"
        )
    )
)

if not defined QT_DIR (
    echo [error] No Qt 6 msvc2022_64 install was found under C:\Qt.
    echo         Set NOVUUI_QT to the kit directory, for example:
    echo             set NOVUUI_QT=C:\Qt\6.10.3\msvc2022_64
    set "RC=1"
    goto :end
)
if not exist "%QT_DIR%\bin\Qt6Core.dll" (
    echo [error] "%QT_DIR%" does not look like a Qt kit - no bin\Qt6Core.dll.
    set "RC=1"
    goto :end
)

rem --- configure and build ---------------------------------------------------
if not exist "%CACHE%" (
    echo Configuring against %QT_DIR% ...
    "%NOVUUI_CMAKE%" -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="%QT_DIR%"
    if errorlevel 1 (
        echo [error] Configure failed.
        set "RC=1"
        goto :end
    )
)

echo Building %CONFIG% ...
"%NOVUUI_CMAKE%" --build "%BUILD_DIR%" --config %CONFIG%
if errorlevel 1 (
    echo [error] Build failed.
    set "RC=1"
    goto :end
)

set "PATH=%QT_DIR%\bin;%PATH%"

if "%MODE%"=="tests" goto :tests
if "%MODE%"=="build" goto :built
goto :run

rem --- clean -----------------------------------------------------------------
:clean
if exist "%BUILD_DIR%" (
    echo Removing %BUILD_DIR% ...
    rmdir /s /q "%BUILD_DIR%"
)
echo Clean.
goto :end

:built
echo Built.
goto :end

rem --- tests -----------------------------------------------------------------
rem Through ctest rather than by naming the executables: the suites are already
rem registered by tests/CMakeLists.txt together with the offscreen platform they
rem need, so a suite added later is picked up here for free.
:tests
for %%I in ("%NOVUUI_CMAKE%") do set "CTEST=%%~dpIctest.exe"
if not exist "%CTEST%" (
    echo [error] ctest.exe was not found next to cmake.
    set "RC=1"
    goto :end
)
"%CTEST%" --test-dir "%BUILD_DIR%" -C %CONFIG% --output-on-failure
if not errorlevel 1 goto :end
set "RC=1"

rem ctest says which suite failed but not why: QTest writes its report through
rem its own logger, and on Windows that does not reliably reach a redirected
rem console, so what ctest captures comes back empty. Re-running each suite with
rem an explicit file logger and echoing the failures out of it works whatever
rem the console is doing.
set "QT_QPA_PLATFORM=offscreen"
call :report tst_gallery
call :report tst_window_chrome
goto :end

rem --- run -------------------------------------------------------------------
:run
set "GALLERY=%BUILD_DIR%\playground\%CONFIG%\NovuUIPlayground.exe"
if not exist "%GALLERY%" (
    echo [error] "%GALLERY%" was not built. Is NOVUUI_BUILD_PLAYGROUND off?
    set "RC=1"
    goto :end
)

rem Through START, which is how a batch launcher hands off to a GUI program.
rem The empty "" is START's window title, not a stray argument: without it START
rem would read a quoted program path as the title and never launch anything.
rem
rem The gallery then hands over again, to a private copy of itself under TEMP,
rem so that "Rebuild and reload" can relink the executable in the build tree.
rem That is why the console returns while the window stays up - not a failure.
rem
rem Known wart, measured rather than assumed: that copy inherits this script's
rem stdout, so "run.bat > log.txt" does not return until the gallery window is
rem closed - a redirect cannot finish while a live process holds the handle.
rem START with its streams on NUL does not break the chain either. It costs
rem nothing in practice, because a console or a double-click is unaffected and
rem a script wanting output wants "run.bat build" or "run.bat tests", both of
rem which exit on their own.
echo Starting the gallery ...
start "" "%GALLERY%" %ARGS%
if errorlevel 1 set "RC=1"
goto :end

rem --- end -------------------------------------------------------------------
rem Pause only when something went wrong.
rem
rem The usual trick is to sniff %cmdcmdline% for the script's own name and call
rem that a double-click - but "cmd /c run.bat tests" matches it too, so a script
rem or a CI job would sit at a prompt forever. And on the happy path there is
rem nothing to read: the gallery window is up, or the report has been printed.
rem A vanishing console only loses information when it vanishes on an error.
:end
if not "%RC%"=="0" (
    if not defined NOVUUI_NO_PAUSE (
        echo.
        pause
    )
)
exit /b %RC%

rem ---------------------------------------------------------------------------
rem Prints the failing assertions of one suite. A subroutine rather than another
rem nested block: batch parses a for-loop inside two ifs inside a block by its
rem own rules, and the version that did this inline silently ran nothing at all.
:report
set "EXE=%BUILD_DIR%\tests\%CONFIG%\%~1.exe"
if not exist "%EXE%" exit /b 0
set "REPORT=%BUILD_DIR%\%~1-report.txt"
"%EXE%" -o "%REPORT%,txt" >nul 2>&1
findstr /b /c:"FAIL!" "%REPORT%" >nul 2>&1
if errorlevel 1 exit /b 0
echo.
echo --- %~1 ---
findstr /b /c:"FAIL!" "%REPORT%"
echo     full report: %REPORT%
exit /b 0
