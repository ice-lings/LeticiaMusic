cmake_minimum_required(VERSION 3.16)

project(LeticiaMusic VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 默认构建类型 (QtCreator Android Kit 需要此变量匹配 "Build type" 字段)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep Qt's AUTOGEN targets off third-party libraries; enable them only for our targets below.
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Quick
    Gui
    Multimedia
    Sql
    Network
)


# ========== TagLib 配置 ==========
set(TAGLIB_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/thirdparty/taglib)
set(TAGLIB_BINARY_DIR ${CMAKE_BINARY_DIR}/src/thirdparty/taglib)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(ENABLE_STATIC OFF CACHE BOOL "Disable static library" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(BUILD_BINDINGS OFF CACHE BOOL "Disable TagLib C bindings" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
set(TAGLIB_ENCODING_FROM_UTF8 ON CACHE BOOL "" FORCE)

add_subdirectory(src/thirdparty/taglib)

# ========== libcurl 配置 ==========
set(CURL_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/thirdparty/curl)
set(CURL_BINARY_DIR ${CMAKE_BINARY_DIR}/src/thirdparty/curl)

set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CURL_ENABLE_SSL ON CACHE BOOL "" FORCE)
set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)

# == OpenSSL 路径配置（按平台区分）===============================
#   优先级：硬编码值 → 环境变量 → 报错中断
#   填写为空则自动从环境变量获取
# ==================================================================
set(ANDROID_OPENSSL_DIR "" CACHE PATH "Android OpenSSL 预编译库目录（环境变量: ANDROID_OPENSSL_DIR）")

if(ANDROID)
    if(NOT ANDROID_OPENSSL_DIR OR ANDROID_OPENSSL_DIR STREQUAL "")
        if(DEFINED ENV{ANDROID_OPENSSL_DIR})
            set(ANDROID_OPENSSL_DIR $ENV{ANDROID_OPENSSL_DIR})
        else()
            message(FATAL_ERROR "请设置 ANDROID_OPENSSL_DIR 或环境变量 ANDROID_OPENSSL_DIR")
        endif()
    endif()
    set(OPENSSL_ROOT_DIR "${ANDROID_OPENSSL_DIR}/ssl_3/${CMAKE_ANDROID_ARCH_ABI}" CACHE PATH "OpenSSL root directory")
    set(OPENSSL_INCLUDE_DIR "${ANDROID_OPENSSL_DIR}/ssl_3/include" CACHE PATH "OpenSSL include directory" FORCE)
    set(OPENSSL_SSL_LIBRARY "${ANDROID_OPENSSL_DIR}/ssl_3/${CMAKE_ANDROID_ARCH_ABI}/libssl.a" CACHE FILEPATH "Path to libssl" FORCE)
    set(OPENSSL_CRYPTO_LIBRARY "${ANDROID_OPENSSL_DIR}/ssl_3/${CMAKE_ANDROID_ARCH_ABI}/libcrypto.a" CACHE FILEPATH "Path to libcrypto" FORCE)

    get_filename_component(_qt6_cmake_dir "${Qt6_DIR}" PATH)
    get_filename_component(_qt6_arch_prefix "${_qt6_cmake_dir}/../.." ABSOLUTE)
    set(_qt6_priv_include "${_qt6_arch_prefix}/include/QtCore/${Qt6_VERSION}")
    if(EXISTS "${_qt6_priv_include}")
        include_directories("${_qt6_priv_include}")
    else()
        message(WARNING "Qt Android 私有头文件目录未找到: ${_qt6_priv_include}，跳过")
    endif()
else()
    set(OPENSSL_ROOT_DIR "" CACHE PATH "OpenSSL 根目录（环境变量: OPENSSL_ROOT_DIR）")
    if(NOT OPENSSL_ROOT_DIR OR OPENSSL_ROOT_DIR STREQUAL "")
        if(DEFINED ENV{OPENSSL_ROOT_DIR})
            set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR})
        endif()
    endif()
endif()

# 仅启用 FTP 协议 (减小体积)
set(CURL_DISABLE_HTTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TELNET ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_DICT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_FILE ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TFTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_RTSP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_POP3 ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IMAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_GOPHER ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_MQTT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IPFS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMB ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_WEBSOCKETS ON CACHE BOOL "" FORCE)

add_subdirectory(src/thirdparty/curl)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

FILE(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/core/*.h")
FILE(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/core/*.cpp")

# 排除平台专属源文件
if(NOT ANDROID)
    list(FILTER SOURCE_FILES EXCLUDE REGEX "src/core/platform/android_media_scanner")
    list(FILTER HEADER_FILES EXCLUDE REGEX "src/core/platform/android_media_scanner")
endif()

set(CORE_SOURCES ${HEADER_FILES} ${SOURCE_FILES})

# ========== TagLib 头文件包含配置 ==========
file(GLOB_RECURSE ALL_HEADER_FILES
    "${CMAKE_SOURCE_DIR}/src/thirdparty/taglib/*.h"
    "${CMAKE_SOURCE_DIR}/src/thirdparty/taglib/*.hpp"
)

set(INCLUDE_DIRS "")
foreach(_headerFile ${ALL_HEADER_FILES})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()

list(REMOVE_DUPLICATES INCLUDE_DIRS)

# ========== libcurl 头文件包含配置 ==========
list(APPEND INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src/thirdparty/curl/include")

# Debug 模式下默认编译测试（core 编译为静态库 core_lib），Release 下默认关闭
set(BUILD_TESTS_DEFAULT OFF)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(BUILD_TESTS_DEFAULT ON)
endif()
option(BUILD_TESTS "Build test executables (auto-enabled in Debug)" ${BUILD_TESTS_DEFAULT})

if(BUILD_TESTS)
    add_library(core_lib STATIC ${CORE_SOURCES})
    target_include_directories(core_lib PUBLIC ${INCLUDE_DIRS})
    target_link_libraries(core_lib PRIVATE Qt6::Core Qt6::Gui Qt6::Sql Qt6::Multimedia Qt6::Network tag libcurl_static)
    set(APP_CORE_SOURCES "")
    set(APP_CORE_LIB core_lib)
else()
    set(APP_CORE_SOURCES ${CORE_SOURCES})
    set(APP_CORE_LIB "")
endif()


qt_add_executable(LeticiaMusic main.cpp ${APP_CORE_SOURCES})

FILE(GLOB_RECURSE COMMON_QML_FILES "${CMAKE_SOURCE_DIR}/src/qml/common/*.qml")

if(WIN32)
    FILE(GLOB_RECURSE PLATFORM_QML_FILES "${CMAKE_SOURCE_DIR}/src/qml/win/*.qml")
elseif(ANDROID)
    FILE(GLOB_RECURSE PLATFORM_QML_FILES "${CMAKE_SOURCE_DIR}/src/qml/android/*.qml")
else()
    message(FATAL_ERROR "Unsupported platform for QML files")
endif()

list(APPEND PLATFORM_QML_FILES ${COMMON_QML_FILES})

set(RELATIVE_QML_FILES "")
foreach(FILE_PATH ${PLATFORM_QML_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR} ${FILE_PATH})
    list(APPEND RELATIVE_QML_FILES ${REL_PATH})
endforeach()

qt_add_qml_module(LeticiaMusic
    URI LeticiaMusic
    VERSION 1.0
    QML_FILES ${RELATIVE_QML_FILES}
    RESOURCES res.qrc
)

target_link_libraries(LeticiaMusic
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Sql
        Qt6::Network
        tag
        libcurl_static
        ${APP_CORE_LIB}
)

# 安卓 APK 打包时包含 OpenSSL .so 运行时库
if(ANDROID)
    set_target_properties(LeticiaMusic PROPERTIES
        QT_ANDROID_EXTRA_LIBS
        "${ANDROID_OPENSSL_DIR}/ssl_3/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_3.so;${ANDROID_OPENSSL_DIR}/ssl_3/${CMAKE_ANDROID_ARCH_ABI}/libssl_3.so"
    )
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
    if(_build_type_lower STREQUAL "debug")
        set(ANDROID_DEBUG_RES_DIR "${CMAKE_CURRENT_BINARY_DIR}/android_debug_res")
        file(COPY "${CMAKE_SOURCE_DIR}/android/" DESTINATION "${ANDROID_DEBUG_RES_DIR}")
        file(WRITE "${ANDROID_DEBUG_RES_DIR}/res/values/strings.xml"
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<resources>\n    <string name=\"app_name\">LeticiaMusic (Debug)</string>\n</resources>\n"
        )
        file(READ "${ANDROID_DEBUG_RES_DIR}/AndroidManifest.xml" MANIFEST_CONTENT)
        string(REPLACE "package=\"org.qtproject.example.LeticiaMusic\""
                "package=\"org.qtproject.example.LeticiaMusic.debug\""
                MANIFEST_CONTENT "${MANIFEST_CONTENT}")
        file(WRITE "${ANDROID_DEBUG_RES_DIR}/AndroidManifest.xml" "${MANIFEST_CONTENT}")
        set_target_properties(LeticiaMusic PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_DEBUG_RES_DIR}"
        )
    else()
        set_target_properties(LeticiaMusic PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android"
        )
    endif()
endif()

target_include_directories(LeticiaMusic PRIVATE ${INCLUDE_DIRS})

set_target_properties(LeticiaMusic PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "LeticiaMusic"
)

include(GNUInstallDirs)
install(TARGETS LeticiaMusic
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(BUILD_TESTS)
    add_subdirectory(tests/manual)
endif()
