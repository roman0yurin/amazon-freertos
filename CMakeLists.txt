cmake_minimum_required(VERSION 3.7)

PROJECT(rtos_rep C CXX ASM)

get_property(RTOS_PORT GLOBAL PROPERTY RTOS_PORT)
get_property(RTOS_HEAP_TYPE GLOBAL PROPERTY RTOS_HEAP_TYPE)
get_property(RTOS_CONFIG_INCL GLOBAL PROPERTY RTOS_CONFIG_INCL)
get_property(RTOS_POSIX_PORT_INCL GLOBAL PROPERTY RTOS_POSIX_PORT_INCL)

include_directories(lib/include lib/include/private lib/FreeRTOS/portable${RTOS_PORT} ${RTOS_CONFIG_INCL} lib/FreeRTOS-Plus-POSIX/include lib/FreeRTOS-Plus-POSIX/include/portable/${RTOS_POSIX_PORT_INCL})

additionalImport(${CMAKE_CURRENT_SOURCE_DIR}/lib/include)
additionalImport(${RTOS_CONFIG_INCL})
additionalImport(${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Plus-POSIX/include/portable/${RTOS_POSIX_PORT_INCL})
additionalImport(${CMAKE_CURRENT_SOURCE_DIR}/lib/include/private)
additionalImport(${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS/portable${RTOS_PORT})

file(GLOB FREE_RTOS_SRC lib/FreeRTOS/*.c lib/FreeRTOS-Plus-POSIX/source/*.c)
file(GLOB SOURCES
				lib/FreeRTOS/portable${RTOS_PORT}/*.c
				lib/FreeRTOS/portable/MemMang/heap_${RTOS_HEAP_TYPE}.c)
additionalSources("${FREE_RTOS_SRC}")
additionalSources("${SOURCES}")
configureModule()