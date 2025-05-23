cmake_minimum_required(VERSION 3.16.3)
project(daedalus)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(LLVM REQUIRED CONFIG)

message(
	"LLVM:
	Definitions ${LLVM_DEFINITIONS}
	Includes ${LLVM_INCLUDE_DIRS}
	Libraries ${LLVM_LIBRARY_DIRS}
	Targets ${LLVM_TARGETS_TO_BUILD}
	"
)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories("include/")
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Build
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fvisibility-inlines-hidden" SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)

if (${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

add_subdirectory(lib)
add_subdirectory(tests)
