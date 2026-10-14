cmake_minimum_required(VERSION 3.25)
project(openre4 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opções do backend de plataforma Aurora
set(AURORA_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(AURORA_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(AURORA_ENABLE_RMLUI OFF CACHE BOOL "" FORCE)

# Ajustes de dependências do SDL3 no Linux
set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
set(SDL_ALSA OFF CACHE BOOL "" FORCE)

# Adiciona o Aurora
add_subdirectory(vendor/aurora)

# Executável do OpenRE4 Nativo
add_executable(openre4
    src/platform/main_native.cpp
)

target_include_directories(openre4 PRIVATE
    include
    src
    src/game
    vendor/aurora/include
)

target_link_libraries(openre4 PRIVATE
    aurora::core
    aurora::main
    aurora::gx
    aurora::vi
    aurora::pad
    aurora::os
    aurora::dvd
)

message(STATUS "OpenRE4 configurado com sucesso para compilação nativa.")
