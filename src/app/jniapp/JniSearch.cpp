#include "JniSearch.h"
#include <ftslib/indexer.hpp>
#include <ftslib/parser.hpp>
#include <ftslib/searcher.hpp>
#include <iostream>
#include <cstring>

/*
 * Class:     JniSearch
 * Method:    search
 * Signature:
 * (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_JniSearch_search(JNIEnv *env, jclass cl,
                                                jstring config_path,
                                                jstring index_path,
                                                jstring query) {
  std::string c_path = env->GetStringUTFChars(config_path, NULL);
  std::string i_path = env->GetStringUTFChars(index_path, NULL);
  std::string q = env->GetStringUTFChars(query, NULL);

  fts::Config config(c_path);

  const auto *index_data = fts::mmap_bin_file(i_path + "/binary/binary");
  fts::Header header(index_data);
  fts::BinaryIndexAccessor index_accessor(index_data, header);
  std::string result;
  try {
    const auto results = fts::search(config, index_accessor, q);
    result = fts::getStringSearchResult(results);
  } catch (const std::exception &e) {
    result = e.what();
  }

  return env->NewStringUTF(result.c_str());
}
