set(target_name fts)

add_library(${target_name} STATIC
  ftslib/parser.cpp
  ftslib/parser.hpp
  ftslib/indexer.cpp
  ftslib/indexer.hpp
  ftslib/searcher.cpp
  ftslib/searcher.hpp)

include(CompileOptions)
set_compile_options(${target_name})

target_include_directories(
  ${target_name}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(
  ${target_name}
  PRIVATE
    nlohmann_json
    picosha2
)


