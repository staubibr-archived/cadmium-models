cmake_minimum_required(VERSION 3.17)
project(Geo_Hospital)

set(CMAKE_CXX_STANDARD 14)

add_executable(EmergencyGenerator_Test ./test/EmergencyGenerator_Test.cpp)
add_executable(EmergencyProcessor_Test ./test/EmergencyProcessor_Test.cpp)