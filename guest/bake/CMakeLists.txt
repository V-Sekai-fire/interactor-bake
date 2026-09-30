# bake.elf: RFD 2284's bake stage, the CPU part, as a godot-sandbox guest.
# Standalone; transport-meshing-pen's host build composes it the same way.
cmake_minimum_required(VERSION 3.16)
project(bake_guest C CXX)
set(SANDBOX_API "" CACHE PATH "godot-sandbox guest API checkout (contract-guest-runtime's sandbox-api)")
set(DOWNLOAD_RUNTIME_API OFF CACHE BOOL "" FORCE)
set(SANDBOX_RISCV_EXT_V OFF CACHE BOOL "" FORCE)
add_subdirectory(${SANDBOX_API}/cmake sandbox_api)
add_sandbox_program(bake main.cpp bake.cpp)
target_link_libraries(bake PRIVATE -lm -lstdc++)
