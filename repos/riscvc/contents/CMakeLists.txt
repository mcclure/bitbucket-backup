cmake_minimum_required(VERSION 3.10)

set(RISCV_SRC
    main.cpp
    execute.cpp
)

if (NOT DEFINED TEX_PATH)
	message(FATAL_ERROR "Must set TEX_PATH to a path, to the src/ directory in the https://github.com/riscv/riscv-isa-manual repo")
endif()

add_custom_command(OUTPUT execute.cpp opcode.h
	       COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/helper/opcode-generate.pl ${TEX_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/execute-input.cpp ${CMAKE_CURRENT_BINARY_DIR}/execute.cpp ${CMAKE_CURRENT_BINARY_DIR}/opcode.h
	       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/execute-input.cpp ${CMAKE_CURRENT_SOURCE_DIR}/helper/opcode-generate.pl)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(riscv ${RISCV_SRC})
