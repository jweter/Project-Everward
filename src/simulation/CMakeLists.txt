cmake_minimum_required(VERSION 3.20)
project(EverwardSimulation LANGUAGES CXX)

add_library(everward_simulation INTERFACE)
target_include_directories(everward_simulation INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(everward_simulation INTERFACE cxx_std_20)

include(CTest)
if(BUILD_TESTING)
    add_executable(everward_simulation_tests tests/simulation_core_tests.cpp)
    target_link_libraries(everward_simulation_tests PRIVATE everward_simulation)
    add_test(NAME everward_simulation_tests COMMAND everward_simulation_tests)

    add_executable(everward_evolution_tests tests/evolution_generator_tests.cpp)
    target_link_libraries(everward_evolution_tests PRIVATE everward_simulation)
    add_test(NAME everward_evolution_tests COMMAND everward_evolution_tests)
endif()
