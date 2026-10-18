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

add_definitions(-DTARGET_PC=1 -DOPENRE4_NATIVE=1)
add_compile_options(-g -fpermissive -Wno-changes-meaning -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-narrowing -Wno-register)

file(GLOB GAME_SOURCES src/game/*.cpp)
file(GLOB ST1_SOURCES src/st1/*.cpp)
set(ST_SOURCES
    src/st/em_wrap.cpp
    src/st/cSceObj.cpp
)
set(WEP_SOURCES
    src/platform/wep_native.cpp
    src/wep/pl_handgun.cpp
    src/wep/objRuger.cpp
    src/wep02/objMauser.cpp
    src/wep02/wep02.cpp
)
set(EM_SOURCES
    src/platform/em_native.cpp
    src/em10/em10.cpp
    src/em12/em12_set.cpp
    src/em15/em15_set.cpp
    src/em26/em26.cpp
)

# Splash de boot: converte assets/boot.png em RGBA cru no momento do build.
find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_FOUND)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/boot_assets/boot.rgba ${CMAKE_BINARY_DIR}/boot_assets/boot.meta
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/make_boot_rgba.py ${CMAKE_BINARY_DIR}/boot_assets
        DEPENDS ${CMAKE_SOURCE_DIR}/assets/boot.png ${CMAKE_SOURCE_DIR}/scripts/make_boot_rgba.py
        COMMENT "Gerando splash de boot (boot.rgba)"
    )
    add_custom_target(openre4_boot_assets ALL
        DEPENDS ${CMAKE_BINARY_DIR}/boot_assets/boot.rgba ${CMAKE_BINARY_DIR}/boot_assets/boot.meta
    )
endif ()

# Executável do OpenRE4 Nativo
add_executable(openre4
    src/platform/main_native.cpp
    src/platform/boot_splash.cpp
    src/platform/menu_native.cpp
    src/platform/dolphin_shims.cpp
    src/platform/gx_native.cpp
    ${GAME_SOURCES}
    ${ST1_SOURCES}
    ${ST_SOURCES}
    ${WEP_SOURCES}
    ${EM_SOURCES}
)

target_include_directories(openre4 PRIVATE
    vendor/aurora/include
    ${CMAKE_BINARY_DIR}/_deps/sdl-src/include
    include
    src
    src/game
    src/st1
    src/wep
    src/wep02
    src/em10
    src/em12
    src/em15
    src/em26
)

target_link_libraries(openre4 PRIVATE
    aurora::core
    aurora::main
    aurora::gx
    aurora::vi
    aurora::pad
    aurora::mtx
    aurora::dvd
    aurora::card
    aurora::thp
    aurora::os
    ${CMAKE_SOURCE_DIR}/crates/openre4_ui/target/release/libopenre4_ui.so
    dl
    pthread
)

# Módulo de UI em Rust (crates/openre4_ui)
find_program(CARGO_EXECUTABLE cargo REQUIRED)
set(OPENRE4_UI_LIB ${CMAKE_SOURCE_DIR}/crates/openre4_ui/target/release/libopenre4_ui.so)
add_custom_command(
    OUTPUT ${OPENRE4_UI_LIB}
    COMMAND ${CARGO_EXECUTABLE} build --release --manifest-path ${CMAKE_SOURCE_DIR}/crates/openre4_ui/Cargo.toml
    DEPENDS ${CMAKE_SOURCE_DIR}/crates/openre4_ui/src/lib.rs ${CMAKE_SOURCE_DIR}/crates/openre4_ui/Cargo.toml
    COMMENT "Compilando crate openre4_ui em Rust"
)
add_custom_target(openre4_ui_target ALL
    DEPENDS ${OPENRE4_UI_LIB}
)
add_dependencies(openre4 openre4_ui_target)

set_target_properties(openre4 PROPERTIES
    BUILD_RPATH "${CMAKE_SOURCE_DIR}/crates/openre4_ui/target/release"
    INSTALL_RPATH "${CMAKE_SOURCE_DIR}/crates/openre4_ui/target/release"
)

message(STATUS "OpenRE4 configurado com sucesso para compilação nativa.")
