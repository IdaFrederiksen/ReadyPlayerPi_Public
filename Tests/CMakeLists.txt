cmake_minimum_required(VERSION 3.16)
project(ReadyPlayerPiTests VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)

set(NOTEPI_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../ClientApp/NotePi)
set(SHARED_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../Shared)
set(KISSFFT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/kissfft)

# ── module_test: DSP-pipeline mod laptop-mic (eksisterende test) ──────────────
add_executable(module_test
    NotePiModuleTest.cpp

    ${NOTEPI_DIR}/boundary/MicrophoneDriver.cpp
    ${NOTEPI_DIR}/control/SoundAnalyzer.cpp
    ${NOTEPI_DIR}/control/SampleProcessor.cpp
    ${NOTEPI_DIR}/control/FirFilter.cpp
    ${NOTEPI_DIR}/control/FftAnalyzer.cpp
    ${NOTEPI_DIR}/control/NoteDetector.cpp
    ${NOTEPI_DIR}/domain/NoteSequence.cpp

    ${KISSFFT_DIR}/kiss_fft.c
    ${KISSFFT_DIR}/kiss_fftr.c
)

target_include_directories(module_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${NOTEPI_DIR}
    ${NOTEPI_DIR}/control
    ${NOTEPI_DIR}/boundary
    ${NOTEPI_DIR}/domain
    ${KISSFFT_DIR}
)

target_link_libraries(module_test PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    PkgConfig::ALSA
    m
)

# ── notepi_end_to_end_test: server + 2 sockets + protokol-roundtrip ──────────
# Forks ./build/server, åbner host+guest sockets, driver lobby + Wordle-spil
# manuelt via JSON. Verificerer at game_over modtages.
# Ingen ALSA, ingen Qt — ren protokol-test.
add_executable(notepi_end_to_end_test
    NotePiEndToEndTest.cpp
)

target_include_directories(notepi_end_to_end_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHARED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(notepi_end_to_end_test PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
)
