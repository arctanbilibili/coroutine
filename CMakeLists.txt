cmake_minimum_required(VERSION 3.0.0)
project(timer VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(${PROJECT_SOURCE_DIR})
aux_source_directory(${PROJECT_SOURCE_DIR} CO_SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/include CO_SRC_LIST)
# aux_source_directory(${PROJECT_SOURCE_DIR} CO_SRC_LIST)

message(${CO_SRC_LIST})

add_executable(timer ${CO_SRC_LIST})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
