#include <ftslib/parser.hpp>
#include <gtest/gtest.h>

TEST(ParserTest, ParseTest1Spcs) {
  try {
    const std::string text = "                \t";
    const fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");
    const std::vector<fts::ParsedString> result = fts::parse(text, config);
    EXPECT_EQ(int(result.size()), 0);
  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}

TEST(ParserTest, ParseTest2NrmlExmpl) {
  try {
    const std::string text =
        "Finished with my woman, сause she couldn't help me with my mind..";
    const std::string expected_ngrams[] = {
        "fin",   "fini",   "finis", "finish", "wom", "woma", "woman",
        "сa",    "сau",    "сaus",  "сause",  "she", "cou",  "coul",
        "could", "couldn", "hel",   "help",   "min", "mind"};
    const int expected_positions[] = {0, 2, 3, 4, 5, 6, 9};
    const fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");
    const std::vector<fts::ParsedString> result = fts::parse(text, config);
    int i = 0, j = 0;
    for (const auto &word : result) {
      for (const auto &ngram : word.word_ngrams) {
        EXPECT_EQ(ngram, expected_ngrams[j]);
        ++j;
      }
      EXPECT_EQ(int(word.word_position), expected_positions[i]);
      ++i;
    }
    EXPECT_EQ(int(result.size()), i);
  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}

TEST(ParserTest, ParseTest3NrmlExmpl) {
  try {
    std::string text = "There's something happening here..";
    std::string expected_ngrams[] = {
        "the",    "ther", "there", "theres", "som",    "some", "somet",
        "someth", "hap",  "happ",  "happe",  "happen", "her",  "here"};
    int expected_positions[] = {0, 1, 2, 3};
    fts::Config config =
        fts::Config(std::filesystem::current_path() / "config.json");
    std::vector<fts::ParsedString> result = fts::parse(text, config);
    int i = 0, j = 0;
    for (const auto &word : result) {
      for (const auto &ngram : word.word_ngrams) {
        EXPECT_EQ(ngram, expected_ngrams[j]);
        ++j;
      }
      EXPECT_EQ(int(word.word_position), expected_positions[i]);
      ++i;
    }
    EXPECT_EQ(int(result.size()), i);
  } catch (fts::ConfigurationException &e) {
    std::cerr << e.what() << "\n";
  };
}
