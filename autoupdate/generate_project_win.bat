perl generate_app_info.pl sven_internal sven_internal.dll 2 0 27

IF NOT EXIST "project" (
	mkdir project
)

cd project

cmake .. -A Win32

pause
