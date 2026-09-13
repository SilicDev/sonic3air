@echo on

call ../get_msbuild_path.bat


:: Build ixwebsocket

@echo.
@echo.
@echo === Building ixwebsocket ===

if not exist .\ixwebsocket\build  mkdir .\ixwebsocket\build
cmake -G "Visual Studio 17 2022" -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" -S .\ixwebsocket -B .\ixwebsocket\build

pushd ixwebsocket\build
%msbuildPath% ixwebsocket.sln /target:ixwebsocket /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% ixwebsocket.sln /target:ixwebsocket /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
pushd mbedtls_bin
%msbuildPath% "Mbed TLS.sln" /target:mbedtls /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% "Mbed TLS.sln" /target:mbedtls /property:Configuration=Release   /property:Platform=x64 -verbosity:minimal
popd
popd

if not exist .\ixwebsocket\buildWin32  mkdir .\ixwebsocket\buildWin32
cmake -G "Visual Studio 17 2022" -DCMAKE_GENERATOR_PLATFORM=Win32 -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" -S .\ixwebsocket -B .\ixwebsocket\buildWin32

pushd ixwebsocket\buildWin32
%msbuildPath% ixwebsocket.sln /target:ixwebsocket /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% ixwebsocket.sln /target:ixwebsocket /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
pushd mbedtls_bin
%msbuildPath% "Mbed TLS.sln" /target:mbedtls /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% "Mbed TLS.sln" /target:mbedtls /property:Configuration=Release   /property:Platform=Win32 -verbosity:minimal
popd
popd

call copy_ixwebsocket.bat no_pause


:: Done
echo.
if "%1"=="" pause