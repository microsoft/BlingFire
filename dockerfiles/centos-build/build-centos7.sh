rm -rf ../../centos7release

docker build . -t blingfire-centos7-build

docker run -it \
    --mount type=bind,source="$(pwd)/../..",target=/BlingFire \
    docker.io/library/blingfire-centos7-build /bin/bash -c \
    "cd /BlingFire && mkdir centos7release && cd centos7release && cmake3 -DCMAKE_BUILD_TYPE=Release .. && make"

cp centos7release/libblingfiretokdll.so ../nuget/lib/centos7