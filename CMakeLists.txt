cmake_minimum_required(VERSION 3.15)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

project(QEng CXX)

file(GLOB LIBRARY_SOURCE_FILES source/library/src/*.cpp)
file(GLOB LIBRARY_HEADER_FILES source/library/inc/*.h*)

add_library(qeng SHARED ${LIBRARY_SOURCE_FILES} ${LIBRARY_HEADER_FILES})

add_executable(main source/drivers/main.cpp)

# Set the path to the TA-Lib include directory
target_include_directories(qeng PUBLIC source/library/inc source/externals/ta-lib/include)

# Set the path to the TA-Lib library directory
#target_link_directories(qeng PUBLIC source/externals/ta-lib/lib)

# Link against the TA-Lib library
set(TA_LIB_PATH ${CMAKE_SOURCE_DIR}/source/externals/ta-lib/lib)
target_link_libraries(qeng PUBLIC ${TA_LIB_PATH}/libta_abstract_cmd.a)
target_link_libraries(qeng PUBLIC ${TA_LIB_PATH}/libta_common_cmd.a)
target_link_libraries(qeng PUBLIC ${TA_LIB_PATH}/libta_func_cmd.a)

target_include_directories(main PUBLIC source/library/inc source/externals/ta-lib/include)

#target_link_directories(main PUBLIC source/externals/ta-lib/lib)

target_link_libraries(main PUBLIC qeng)
