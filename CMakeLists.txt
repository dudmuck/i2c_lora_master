
cmake_minimum_required(VERSION 2.6)
project (lit)

add_library(i2c_lora STATIC
	i2c_lora.cpp
	cmds.c
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DCONF_DIR=\"\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"\"")

list(APPEND test_srcs
    test.cpp
)

include_directories("${PROJECT_SOURCE_DIR}")

add_executable(testI2C ${test_srcs})

target_link_libraries(i2c_lora wiringPi)
target_link_libraries(testI2C i2c_lora json-c)

####################################################
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(i2c_lora.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(i2c_lora.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_MODULE(i2c_lora python i2c_lora.i i2c_lora.cpp cmds.c)
SWIG_LINK_LIBRARIES(i2c_lora ${PYTHON_LIBRARIES} wiringPi)
