#include <cxxopts.hpp>
#include <ftslib/indexer.hpp>
#include <ftslib/parser.hpp>
#include <ftslib/searcher.hpp>
#include <iostream>
#include <replxx.hxx>

void start_search(const fts::Config &config,
                  const std::filesystem::path &index_path,
                  const std::string &query) {
  try {
    const auto *index_data = fts::mmap_bin_file(index_path / "binary/binary");
    fts::Header header(index_data);
    fts::BinaryIndexAccessor acsor_to_idx(index_data, header);
    const auto result = fts::search(config, acsor_to_idx, query);
    fts::printResult(result);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
  }
}

void start_search_interactive(const fts::Config &config,
                              const std::filesystem::path &index_path) {
  replxx::Replxx editor;
  editor.clear_screen();
  while (true) {
    char const *cinput{nullptr};
    do {
      cinput = editor.input("> ");
    } while ((cinput == nullptr) && (errno == EAGAIN));
    if (cinput == nullptr) {
      break;
    }
    if (strcmp(cinput, "!q") == 0) {
      break;
    }
    std::string query = cinput;
    if (query.empty()) {
      continue;
    }
    try {
      start_search(config, index_path, query);
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      break;
    }
  }
}

int main(int argc, char **argv) {
  cxxopts::Options options("lab5", "searcher");
  fts::Config config(std::filesystem::current_path() / "config.json");

  try {
    // clang-format off

    options.add_options()
      ("index", "json file", cxxopts::value<std::string>())
      ("query", "text to parce", cxxopts::value<std::string>()->default_value("__query_"));
    // clang-format on

    const auto result = options.parse(argc, argv);

    const auto index = result["index"].as<std::string>();
    const auto query = result["query"].as<std::string>();

    if (query == "__query_") {
      start_search_interactive(config, index);
    } else {
      start_search(config, index, query);
    }

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
