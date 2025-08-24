@echo off

if "%VK_SDK_PATH%"=="" (
    echo environment variable VK_SDK_PATH must be set
    exit /b 1
)

"%VK_SDK_PATH%\Bin\glslc.exe" ../assets/shaders/src/vertex.vert -o 			../assets/shaders/intermediate/vertex.spv
"%VK_SDK_PATH%\Bin\glslc.exe" ../assets/shaders/src/fragment.frag -o 		../assets/shaders/intermediate/fragment.spv

pause