set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# smooth_ui_toolkit uses std::this_thread, which is unavailable in Ubuntu's
# MinGW Win32-threading variant. Select the POSIX-threading compilers
# explicitly instead of relying on the host's update-alternatives choice.
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Seed the linker flags once. Appending to the cached CMAKE_EXE_LINKER_FLAGS
# here would duplicate these options every time the build tree is configured.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
