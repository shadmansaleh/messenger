cmake_minimum_required(VERSION 3.6.0)
project(Messenger VERSION 1.0)
#configure_file(MessengerConfig.h.in MessengerConfig.h)
#target_include_directories(Messenger PUBLIC
#                           "${PROJECT_BINARY_DIR}"
#                           )
include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
)
file(GLOB SRCSSERVER
	"${PROJECT_SOURCE_DIR}/include/*"
	"${PROJECT_SOURCE_DIR}/src/server/*"
)
add_executable(server ${SRCSSERVER})
file(GLOB SRCSCLIENT
	"${PROJECT_SOURCE_DIR}/include/*"
	"${PROJECT_SOURCE_DIR}/src/client/*"
)
#add_subdirectory(MessengerHelper)
#file(GLOB SRCSLIB
#	"${PROJECT_SOURCE_DIR}/include/*"
#	"${PROJECT_SOURCE_DIR}/src/lib/*"
#)
#add_library(MessngerHelper ${SRCSLIB})

add_executable(client ${SRCSCLIENT})

#target_link_libraries(messenger PUBLIC MssengerHelper)

