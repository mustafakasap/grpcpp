message(STATUS "***********************************")

cmake_minimum_required(VERSION 3.21.0)
PROJECT(grpcsvc VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -pthread")
set(CMAKE_CXX_FLAGS_DEBUG  "-g")

find_package(Threads REQUIRED)

# Find Protobuf installation
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)


# Find gRPC installation
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)



# Proto file
set(PROTO_FILE_NAME_01	"greeter")
get_filename_component(ext_proto "../proto/${PROTO_FILE_NAME_01}.proto" ABSOLUTE)
get_filename_component(ext_proto_path "${ext_proto}" PATH)

# Generated sources 
set(ext_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_FILE_NAME_01}.pb.cc")
set(ext_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_FILE_NAME_01}.pb.h")
set(ext_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_FILE_NAME_01}.grpc.pb.cc")
set(ext_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_FILE_NAME_01}.grpc.pb.h")

add_custom_command(
	OUTPUT "${ext_proto_srcs}" "${ext_proto_hdrs}" "${ext_grpc_srcs}" "${ext_grpc_hdrs}"
	COMMAND ${_PROTOBUF_PROTOC}
	ARGS	--grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
			--cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
			-I "${ext_proto_path}"
			--plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
			"${ext_proto}" "${inf_proto}" "${med_proto}"
	DEPENDS "${ext_proto}"
)

# Include generated *.pb.h files
include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}" 
	"${PROJECT_SOURCE_DIR}/common" 
)

# Targets
add_executable(
	server
	"server/main.cpp"
	"server/gserver.cpp"
	"server/gsession.cpp"
	"common/log_helper.cpp"
	${ext_proto_srcs}
	${ext_grpc_srcs}
)
target_compile_definitions(server PUBLIC SERVER)
target_link_libraries(
	server
	${_REFLECTION}
	${_GRPC_GRPCPP}
	${_PROTOBUF_LIBPROTOBUF}
)
set_target_properties(
	server
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../out"
	LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../out"
	RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../out"
)  		

add_executable(
	client
	"client/main.cpp"
	"client/gclient.cpp"
	"common/log_helper.cpp"
	${ext_proto_srcs}
	${ext_grpc_srcs}
)
target_compile_definitions(client PUBLIC CLIENT)
target_link_libraries(
	client
	${_REFLECTION}
	${_GRPC_GRPCPP}
	${_PROTOBUF_LIBPROTOBUF}
)
set_target_properties(
	client
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
 
