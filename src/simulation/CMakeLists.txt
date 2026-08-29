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

    add_executable(everward_attitude_control_tests tests/attitude_control_tests.cpp)
    target_link_libraries(everward_attitude_control_tests PRIVATE everward_simulation)
    add_test(NAME everward_attitude_control_tests COMMAND everward_attitude_control_tests)

    add_executable(everward_evolution_tests tests/evolution_generator_tests.cpp)
    target_link_libraries(everward_evolution_tests PRIVATE everward_simulation)
    add_test(NAME everward_evolution_tests COMMAND everward_evolution_tests)

    add_executable(everward_sensorium_tests tests/sensorium_tests.cpp)
    target_link_libraries(everward_sensorium_tests PRIVATE everward_simulation)
    add_test(NAME everward_sensorium_tests COMMAND everward_sensorium_tests)

    add_executable(everward_software_policy_tests tests/software_policy_tests.cpp)
    target_link_libraries(everward_software_policy_tests PRIVATE everward_simulation)
    add_test(NAME everward_software_policy_tests COMMAND everward_software_policy_tests)

    add_executable(everward_subsystem_consequence_tests tests/subsystem_consequence_tests.cpp)
    target_link_libraries(everward_subsystem_consequence_tests PRIVATE everward_simulation)
    add_test(NAME everward_subsystem_consequence_tests COMMAND everward_subsystem_consequence_tests)

    add_executable(everward_contact_physics_tests tests/contact_physics_tests.cpp)
    target_link_libraries(everward_contact_physics_tests PRIVATE everward_simulation)
    add_test(NAME everward_contact_physics_tests COMMAND everward_contact_physics_tests)

    add_executable(everward_compound_contact_tests tests/compound_contact_tests.cpp)
    target_link_libraries(everward_compound_contact_tests PRIVATE everward_simulation)
    add_test(NAME everward_compound_contact_tests COMMAND everward_compound_contact_tests)

    add_executable(everward_impact_damage_tests tests/impact_damage_tests.cpp)
    target_link_libraries(everward_impact_damage_tests PRIVATE everward_simulation)
    add_test(NAME everward_impact_damage_tests COMMAND everward_impact_damage_tests)

    add_executable(everward_manipulator_tests tests/manipulator_tests.cpp)
    target_link_libraries(everward_manipulator_tests PRIVATE everward_simulation)
    add_test(NAME everward_manipulator_tests COMMAND everward_manipulator_tests)
endif()