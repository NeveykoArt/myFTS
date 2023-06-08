set(target_name indexer)

add_executable(${target_name})

include(CompileOptions)
set_compile_options(${target_name})

target_sources(
  ${target_name}
  PRIVATE
    app/index.cpp
)

target_link_libraries(
  ${target_name}
  PRIVATE
    fts
    cxxopts
    rapidcsv
)

set(target_name searcher)

add_executable(${target_name})

include(CompileOptions)
set_compile_options(${target_name})

target_sources(
  ${target_name}
  PRIVATE
    app/search.cpp
)

target_link_libraries(
  ${target_name}
  PRIVATE
    fts
    cxxopts
    replxx
)

find_package(Java REQUIRED)

include(UseJava)

add_jar(javasearcher
    jniapp/main.java
    jniapp/JniSearch.java
    ENTRY_POINT main
    GENERATE_NATIVE_HEADERS JniSearch-native
)

find_package(JNI REQUIRED)

add_library(JniSearch SHARED jniapp/JniSearch.cpp)

target_link_libraries(
  JniSearch
  PRIVATE
    fts
    JNI::JNI
    JniSearch-native
)

