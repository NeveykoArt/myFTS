#pragma once

#include <filesystem>
#include <vector>

namespace fts {

struct ParsedString {
  std::vector<std::string> word_ngrams;
  size_t word_position;
};

class Config {
public:
  explicit Config(const std::filesystem::path &pathJsonFile);
  const std::vector<std::string> &getStopWords() const { return stop_words; }
  size_t getNgramMinLength() const { return ngram_min_length; }
  size_t getNgramMaxLength() const { return ngram_max_length; }

private:
  std::vector<std::string> stop_words;
  size_t ngram_min_length;
  size_t ngram_max_length;
};

class ConfigurationException : public std::runtime_error {
public:
  explicit ConfigurationException(const std::string &what_arg)
      : std::runtime_error(what_arg) {}
};

std::vector<ParsedString> parse(std::string text, const Config &config);

} // namespace fts
