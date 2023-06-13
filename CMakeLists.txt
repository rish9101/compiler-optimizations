project(compiler-optimizations)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)

find_package(LLVM 10 REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(./include)

add_library(localopts MODULE
    ./src/Common.cc
    ./src/LocalOpts.cc
)

set_target_properties(localopts PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fPIC -g"
)

add_library(reachingdefinitions MODULE
    ./src/Common.cc
    ./src/ReachingDefinitions.cc
)

set_target_properties(reachingdefinitions PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fPIC -g"
)

add_library(availableexpressions MODULE
    ./src/Common.cc
    ./src/Expression.cc
    ./src/AvailableExpressions.cc
)

set_target_properties(availableexpressions PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fPIC -g"
)

add_library(livenessanalysis MODULE
    ./src/Common.cc
    ./src/LivenessAnalysis.cc
)

set_target_properties(livenessanalysis PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fPIC -g"
)

add_library(dominators MODULE
    ./src/Common.cc
    ./src/Dominators.cc
)

set_target_properties(dominators PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fPIC -g"
)
