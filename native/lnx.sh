cd build_linux &&\
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_LINUX=True -DDEBUG=True .. &&\
make -j2
