perl generate_app_info.pl sven_internal sven_internal.dll 2 0 27

[ ! -d ./build ] && mkdir build

cd build

cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make
