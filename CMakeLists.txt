project(AMillionRandomDigits)
cmake_minimum_required(VERSION 2.8)

find_package(Qt4 COMPONENTS QtCore)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

add_executable(compress main.cpp)
target_link_libraries(compress ${QT_LIBRARIES})
install(TARGETS compress RUNTIME DESTINATION bin)
