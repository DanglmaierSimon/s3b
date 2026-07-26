:: Initial version created by Chat GPT

@echo off

docker run --rm ^
    -v "%cd%:/workspace" ^
    -v fetchcontent-cache:/cache ^
    -w /workspace ^
    botbuild ^
    ./docker/build_in_docker.sh

pause