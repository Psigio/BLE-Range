cmake_minimum_required(VERSION 3.11.0)
project(BLE-Range VERSION 0.1.0)

SET(CMAKE_CXX_FLAGS "-g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
pkg_check_modules (GOBJECT REQUIRED gobject-2.0)
include_directories(${GOBJECT_INCLUDE_DIRS})
pkg_check_modules (GIO REQUIRED gio-2.0)
include_directories(${GIO_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

get_filename_component(fc_base "../_deps"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
set(FETCHCONTENT_BASE_DIR ${fc_base})
include(FetchContent)
FetchContent_Declare(
        rxcppextras
        GIT_REPOSITORY  https://github.com/Psigio/RxCppExtras.git
        GIT_TAG         HEAD
)
if(${CMAKE_VERSION} VERSION_LESS 3.14)	
    FetchContent_GetProperties(rxcppextras)	
    if(NOT rxcppextras_POPULATED)	
        FetchContent_Populate(rxcppextras)	 
        add_subdirectory(${rxcppextras_SOURCE_DIR} ${rxcppextras_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()	
else()	
    FetchContent_MakeAvailable(rxcppextras)	
endif()

FetchContent_Declare(
        rxcpp
        GIT_REPOSITORY  https://github.com/ReactiveX/RxCpp.git
        GIT_TAG         HEAD
)

FetchContent_GetProperties(rxcpp)
if(NOT rxcpp_POPULATED)
    FetchContent_Populate(rxcpp)
    add_subdirectory(${rxcpp_SOURCE_DIR} ${rxcpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_library(${PROJECT_NAME} STATIC
    include/AdapterProxy.hpp
    include/BluetoothDetector.hpp
    include/BluetoothDetectorApi.hpp
    include/PrivateBluetoothDetectorApi.hpp 
    include/GDBusProxyAuto.hpp
    include/GDBusProxyFactory.hpp
    include/GMainLoopProxy.hpp
    src/AdapterProxy.cpp
    src/BluetoothDetector.cpp
    src/GMainLoopProxy.cpp
    src/PrivateBluetoothDetectorApi.cpp
    src/BluetoothDetectorApiFactory.cpp)


target_include_directories(
    ${PROJECT_NAME}
    PRIVATE include
    PUBLIC ${GLIB_INCLUDE_DIRS}
    PUBLIC ${GOBJECT_INCLUDE_DIRS}
    PUBLIC ${GIO_INCLUDE_DIRS} 
    PRIVATE ${rxcpp_SOURCE_DIR}/Rx/v2/src/   
    PRIVATE ${rxcppextras_SOURCE_DIR}
)

target_link_libraries( 
        ${PROJECT_NAME}
        PUBLIC ${GLIB_LIBRARIES}
        PUBLIC ${GOBJECT_LIBRARIES}
        PUBLIC ${GIO_LIBRARIES}
        Threads::Threads         
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/BluetoothDetectorApi.hpp)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} EXPORT ble-rangeConfig.cmake
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

configure_file(BLE-Range.pc.in BLE-Range.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/BLE-Range.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

export(TARGETS ${PROJECT_NAME} FILE ble-rangeConfig.cmake)
