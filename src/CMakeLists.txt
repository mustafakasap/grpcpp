message(STATUS "***********************************")

cmake_minimum_required(VERSION 3.21.0)
PROJECT(testapp VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -pthread")
set(CMAKE_CXX_FLAGS_DEBUG  "-g")

find_package(Threads REQUIRED)

# Include generated *.pb.h files
include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}" 
)

# Targets
add_executable(
	test
	"main.cpp"
)
target_link_libraries(
	test
	${_REFLECTION}
)
set_target_properties(
	test
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../out"
	LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../out"
	RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../out"
)  		

message(STATUS "-----------------------------------")
message(STATUS "---> CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "---> CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "---> PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "---> CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")