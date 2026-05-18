# CMakeLists.txt
cmake_minimum_required(VERSION 3.13)

# Definitions
project(PigPSO2Cam VERSION 1.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Our stuff
file(GLOB CXX_SRCS "src/*.cpp")

# MinHook
set(MINHOOK_SRCS
    "external/minhook/src/buffer.c"
    "external/minhook/src/hook.c"
    "external/minhook/src/trampoline.c"
    "external/minhook/src/hde/hde32.c"
)

# Imgui
set(IMGUI_SRCS
    "external/imgui/imgui.cpp"
    "external/imgui/imgui_demo.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_impl_dx9.cpp"
    "external/imgui/imgui_impl_win32.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/settings_form.cpp"
)

# ---------------------------------------------------------
# Target: PigPSO2Cam.dll
# ---------------------------------------------------------
# Create the shared library (DLL)
add_library(PigPSO2Cam SHARED ${CXX_SRCS} ${MINHOOK_SRCS} ${IMGUI_SRCS})

# Get rid of "lib" prefix
set_target_properties(PigPSO2Cam PROPERTIES PREFIX "")

# Include directories
target_include_directories(PigPSO2Cam PRIVATE 
    external/minhook/include
    external/imgui/include
)

# Output to bin/
set_target_properties(PigPSO2Cam PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Link libraries derived from the AdditionalDependencies in .vcxproj
target_link_libraries(PigPSO2Cam PRIVATE d3d9 d3dx9 kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32 shlwapi)

# Apply compile-time flags
target_compile_options(PigPSO2Cam PRIVATE 
    -Wall 
    -O2 
    -ffunction-sections 
    -fdata-sections 
)

# Apply linker flags to fix standard call
target_link_options(PigPSO2Cam PRIVATE 
    -static 
    -static-libgcc 
    -static-libstdc++ 
    -Wl,--enable-stdcall-fixup 
    -Wl,--kill-at
    -Wl,--gc-sections
    -flto
    -s
)

