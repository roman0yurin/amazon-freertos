cmake_minimum_required(VERSION 3.7)

PROJECT(rtos_rep C CXX ASM)

get_property(RTOS_PORT GLOBAL PROPERTY RTOS_PORT)
get_property(RTOS_HEAP_TYPE GLOBAL PROPERTY RTOS_HEAP_TYPE)
get_property(RTOS_CONFIG_INCL GLOBAL PROPERTY RTOS_CONFIG_INCL)
get_property(RTOS_POSIX_PORT_INCL GLOBAL PROPERTY RTOS_POSIX_PORT_INCL)

include_directories(lib/include lib/include/private lib/FreeRTOS/portable${RTOS_PORT} ${RTOS_CONFIG_INCL} lib/FreeRTOS-Plus-POSIX/include lib/FreeRTOS-Plus-POSIX/include/portable/${RTOS_POSIX_PORT_INCL})

global_import(${CMAKE_CURRENT_SOURCE_DIR}/lib/include)
global_import(${RTOS_CONFIG_INCL})
global_import(${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Plus-POSIX/include/portable/${RTOS_POSIX_PORT_INCL})
global_import(${CMAKE_CURRENT_SOURCE_DIR}/lib/include/private)
global_import(${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS/portable${RTOS_PORT})

file(GLOB FREE_RTOS_SRC lib/FreeRTOS/*.c lib/FreeRTOS-Plus-POSIX/source/*.c)
file(GLOB FREE_RTOS_PORT_SRC
				lib/FreeRTOS/portable${RTOS_PORT}/*.c
				lib/FreeRTOS/portable/MemMang/heap_${RTOS_HEAP_TYPE}.c)

set_property(GLOBAL PROPERTY RTOS_REP_SRC ${FREE_RTOS_SRC} ${FREE_RTOS_PORT_SRC})
set_property(GLOBAL PROPERTY RTOS_REP_INC
				${CMAKE_CURRENT_SOURCE_DIR}/lib/include
				${CMAKE_CURRENT_SOURCE_DIR}/lib/include/private
				${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS/portable${RTOS_PORT}
)


add_library(${PROJECT_NAME} STATIC ${FREE_RTOS_SRC} ${FREE_RTOS_PORT_SRC})