#include <cxxopts.hpp>
#include <ftslib/indexer.hpp>
#include <ftslib/parser.hpp>
#include <ftslib/searcher.hpp>
#include <iostream>
#include <rapidcsv.h>

struct csvinfo {
  size_t bookid;
  std::string title;
  std::string languagecode;
};

int main(int argc, char **argv) {
  cxxopts::Options options("lab5", "indexer");
  fts::Config config(std::filesystem::current_path() / "config.json");

  try {
    // clang-format off
      options.add_options()
      ("csv", "json file", cxxopts::value<std::string>())
      ("index", "text to parce", cxxopts::value<std::string>());
    // clang-format on

    const auto result = options.parse(argc, argv);

    const auto csv_path = result["csv"].as<std::string>();
    const auto index_path = result["index"].as<std::string>();

    rapidcsv::Document books(csv_path);

    std::vector<csvinfo> parsed_csv_file;
    std::vector<size_t> col_book_id = books.GetColumn<size_t>("bookID");
    std::vector<std::string> col_title = books.GetColumn<std::string>("title");
    std::vector<std::string> col_language_code =
        books.GetColumn<std::string>("language_code");

    fts::IndexBuilder idx;

    auto vsize = col_book_id.size();

    for (size_t i = 0; i < vsize; ++i) {
      parsed_csv_file.push_back(
          {col_book_id[i], col_title[i], col_language_code[i]});
    }

    size_t count = 0;

    for (const auto &[book_id, title, language_code] : parsed_csv_file) {
      if (language_code == "eng" || language_code == "en-US") {
        idx.addDocument(book_id, title, config);
        if (++count % 500 == 0) {
          std::cout << count << " documents...\n";
        }
      }
    }

    fts::BinaryIndexWriter binary_writer;
    binary_writer.write(index_path, idx.getIndex());

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
