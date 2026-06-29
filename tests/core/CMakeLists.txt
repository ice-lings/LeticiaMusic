cmake_minimum_required(VERSION 3.16)

# 测试MusicEntityType
find_package(Qt6 REQUIRED COMPONENTS Core Test)

# 测试目标1: test-musicitem
set(TEST_MUSICITEM_SOURCES
    test-musicitem.cpp
)

add_executable(test-musicitem ${TEST_MUSICITEM_SOURCES})
# WIN32_EXECUTABLE FALSE 使测试输出到控制台
set_target_properties(test-musicitem PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(test-musicitem PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/utils
    ${CMAKE_SOURCE_DIR}/src/thirdparty/taglib/taglib
    ${CMAKE_SOURCE_DIR}/src/thirdparty/taglib/taglib/toolkit
)

target_link_libraries(test-musicitem PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
)

# 测试目标2: test-logger
set(TEST_LOGGER_SOURCES
    test-logger.cpp
)

add_executable(test-logger ${TEST_LOGGER_SOURCES})
# WIN32_EXECUTABLE FALSE 使测试输出到控制台
set_target_properties(test-logger PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(test-logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/utils
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(test-logger PRIVATE
    Qt6::Core
    Qt6::Test
    core_lib
)

# 测试目标3: test-viewmodelmanager
set(TEST_VIEWMANAGER_SOURCES
    test-viewmodelmanager.cpp
    test-viewmodelmanager.h
)

add_executable(test-viewmodelmanager ${TEST_VIEWMANAGER_SOURCES})
# WIN32_EXECUTABLE FALSE 使测试输出到控制台
set_target_properties(test-viewmodelmanager PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(test-viewmodelmanager PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/utils
    ${CMAKE_SOURCE_DIR}/src/core/viewmodels
    ${CMAKE_SOURCE_DIR}/src/core/interfaces
)

target_link_libraries(test-viewmodelmanager PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Sql
    core_lib
)

# 测试目标4: test-sync-compare (compareManifests 单元测试, 无需 NAS)
set(TEST_SYNC_COMPARE_SOURCES
    test-sync-compare.cpp
)

add_executable(test-sync-compare ${TEST_SYNC_COMPARE_SOURCES})
set_target_properties(test-sync-compare PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(test-sync-compare PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/sync
    ${CMAKE_SOURCE_DIR}/src/core/utils
)

target_link_libraries(test-sync-compare PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Network
    core_lib
)

enable_testing()
add_test(NAME test-musicitem COMMAND test-musicitem)
add_test(NAME test-logger COMMAND test-logger)
add_test(NAME test-viewmodelmanager COMMAND test-viewmodelmanager)
add_test(NAME test-sync-compare COMMAND test-sync-compare)

