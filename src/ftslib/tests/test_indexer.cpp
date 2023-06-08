#include <ftslib/indexer.hpp>
#include <ftslib/searcher.hpp>
#include <gtest/gtest.h>
#include <picosha2.h>

TEST(IndexerTest, IndexTest1SameId) {
  try {
    fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");

    fts::IndexBuilder idx;
    idx.addDocument(199903, "The Matrix: 1", config);
    idx.addDocument(199903, "HeheHaha: 2", config);

    std::map<std::string, std::map<size_t, std::vector<size_t>>> expected_entry;
    expected_entry.insert({"mat", {{199903, {0}}}});
    expected_entry.insert({"matr", {{199903, {0}}}});
    expected_entry.insert({"matri", {{199903, {0}}}});
    expected_entry.insert({"matrix", {{199903, {0}}}});

    std::map<size_t, std::string> expected_doc;
    expected_doc = {{199903, "The Matrix: 1"}};

    EXPECT_EQ(idx.getIndex().getDocs(), expected_doc);
    EXPECT_EQ(idx.getIndex().getEntries(), expected_entry);

  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}

TEST(IndexerTest, IndexTest2SameNOD) {
  try {
    fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");

    fts::IndexBuilder idx;
    idx.addDocument(199903, "The Matrix: 1", config);
    idx.addDocument(199904, "The Matrix: 1", config);

    std::map<std::string, std::map<size_t, std::vector<size_t>>> expected_entry;
    expected_entry.insert({"mat", {{199903, {0}}, {199904, {0}}}});
    expected_entry.insert({"matr", {{199903, {0}}, {199904, {0}}}});
    expected_entry.insert({"matri", {{199903, {0}}, {199904, {0}}}});
    expected_entry.insert({"matrix", {{199903, {0}}, {199904, {0}}}});

    std::map<size_t, std::string> expected_doc;
    expected_doc = {{199903, "The Matrix: 1"}, {199904, "The Matrix: 1"}};

    EXPECT_EQ(idx.getIndex().getDocs(), expected_doc);
    EXPECT_EQ(idx.getIndex().getEntries(), expected_entry);

  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}

TEST(IndexerTest, IndexTest3AddDoc) {
  try {
    fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");

    fts::IndexBuilder idx;
    idx.addDocument(199903, "The Matrix: 1", config);
    idx.addDocument(200305, "The Matrix: 2", config);
    idx.addDocument(200311, "The Matrix: 3", config);

    fts::TextIndexWriter writer;
    writer.write(std::filesystem::current_path() / "indextest", idx.getIndex());

    std::map<std::string, std::map<size_t, std::vector<size_t>>> entry;
    std::string hash_hex_term;
    std::string term = "matrix";
    picosha2::hash256_hex_string(term, hash_hex_term);
    fts::parseTextEntry(std::filesystem::current_path() / "indextest" / "text" /"entries" /
                        hash_hex_term.substr(0, 6),
                    entry);

    std::map<std::string, std::map<size_t, std::vector<size_t>>> expected_entry;
    expected_entry.insert(
        {"matrix", {{199903, {1, 0}}, {200305, {1, 0}}, {200311, {1, 0}}}});
    EXPECT_EQ(entry, expected_entry);

  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}

TEST(IndexerTest, IndexTest4BinAdd) {
  try {
    fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");

    fts::IndexBuilder idx;
    idx.addDocument(199903, "The Matrix: 1", config);
    idx.addDocument(200305, "The Matrix: 2", config);
    idx.addDocument(200311, "The Matrix: 3", config);

    fts::BinaryIndexWriter writer;
    writer.write(std::filesystem::current_path() / "indextest", idx.getIndex());

    std::map<size_t, std::vector<size_t>> entry;
    std::string term = "matrix";
    fts::parseBinaryEntry(std::filesystem::current_path() / "indextest" / "binary" / "binary", term, entry);

    std::map<size_t, std::vector<size_t>> expected_entry;
    expected_entry.insert(
        {{4, {1, 0}}, {18, {1, 0}}, {32, {1, 0}}});
    EXPECT_EQ(entry, expected_entry);

  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}
