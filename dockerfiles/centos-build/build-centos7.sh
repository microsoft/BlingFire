docker build . -t blingfire-centos7-build

// Run Docker Image
mkdir Centos7Release
cmake3 -DCMAKE_BUILD_TYPE=Release
make