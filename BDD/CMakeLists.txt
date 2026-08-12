set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(SRC_FILES
    FunctionStep.cpp
    ScenarioRunner.cpp
)

add_library(bdd_runtime STATIC ${SRC_FILES})

target_include_directories(
    bdd_runtime
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(
    bdd_runtime
    PUBLIC
        GTest::gtest
)
