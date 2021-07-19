cmake_minimum_required(VERSION 3.18)
project(mutable-public-api)

set(CMAKE_CXX_STANDARD 17)
include(ExternalProject)
enable_testing()

set(EXECUTABLE_OUTPUT_PATH      "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH         "${PROJECT_BINARY_DIR}/lib")

set(CMAKE_COMPILE_FLAGS             "-W -Wall -pedantic -march=native")
set(CMAKE_C_FLAGS                   "${CMAKE_C_FLAGS} ${CMAKE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} ${CMAKE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb3")

if(APPLE)
    set(MUTABLE_URL "https://bigdata.uni-saarland.de/teaching/dbsys20/mutable-macos-amd64.zip")
    set(MUTABLE_MD5 "afda76f9b29ac9a2268415a64ba2af02")
else()
    set(MUTABLE_URL "https://bigdata.uni-saarland.de/teaching/dbsys20/mutable-linux-amd64.zip")
    set(MUTABLE_MD5 "9550fd3e8926ada9ec1d4a741895acb5")
endif()

ExternalProject_Add(
    Mutable
    PREFIX              "mutable"
    DOWNLOAD_NAME       "mutable.zip"
    URL                 "${MUTABLE_URL}"
    URL_MD5             "${MUTABLE_MD5}"
    DOWNLOAD_NO_EXTRACT 0
    LOG_DOWNLOAD        1
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    BUILD_BYPRODUCTS    "${PROJECT_BINARY_DIR}/mutable/src/Mutable/include"
                        "${PROJECT_BINARY_DIR}/mutable/src/Mutable/lib/libmutable.a"
    INSTALL_COMMAND     ""
)
include_directories(SYSTEM "${PROJECT_BINARY_DIR}/mutable/src/Mutable/include")
add_library(mutable STATIC IMPORTED)
set_target_properties(mutable PROPERTIES IMPORTED_LOCATION "${PROJECT_BINARY_DIR}/mutable/src/Mutable/lib/libmutable.a")
add_dependencies(mutable Mutable)

include_directories(src)
add_subdirectory(src)
add_subdirectory(unittest)
add_subdirectory(benchmark)
