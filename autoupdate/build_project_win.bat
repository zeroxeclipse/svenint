perl generate_app_info.pl sven_internal sven_internal.dll 2 0 27

IF NOT EXIST "build" (
	mkdir build
)

cd build

cmake .. -A Win32
cmake --build . --config Release

pause