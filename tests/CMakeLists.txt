cmake_minimum_required(VERSION 3.6.0)

set(INSTALL_GTEST OFF CACHE BOOL "")

# Download and unpack googletest at configure time
configure_file(${CMAKE_SOURCE_DIR}/contrib/cmake/GoogleTest.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_cmariometroidrt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
# if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories(SYSTEM "${gtest_SOURCE_DIR}/include")
# endif()

add_subdirectory(common)
add_subdirectory(Compress)
add_subdirectory(Encryptions)
add_subdirectory(Hashes)
add_subdirectory(Misc)
add_subdirectory(Packets)
add_subdirectory(PKA)

add_library(TopLevelTests OBJECT
    gpg.cpp
    pgp.cpp
    cleartextsignature.cpp
    detachedsignature.cpp
    key.cpp
    message.cpp
    revocationcertificate.cpp)

file(COPY testvectors DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(OpenPGPTests
    $<TARGET_OBJECTS:TopLevelTests>
    $<TARGET_OBJECTS:CommonTests>
    $<TARGET_OBJECTS:CompressTests>
    $<TARGET_OBJECTS:EncryptionTests>
    $<TARGET_OBJECTS:HashTests>
    $<TARGET_OBJECTS:MiscTests>
    $<TARGET_OBJECTS:PacketTests>
    $<TARGET_OBJECTS:Tag2SubpacketTests>
    $<TARGET_OBJECTS:Tag17SubpacketTests>
    $<TARGET_OBJECTS:PKATests>)

target_link_libraries(OpenPGPTests gtest gtest_main OpenPGP_shared)
set_target_properties(OpenPGPTests PROPERTIES OUTPUT_NAME "tests")

add_test(NAME libOpenPGP COMMAND tests/tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
