#include <cctype>
#include <cstring>
#include <fstream>
#include <ftslib/parser.hpp>
#include <nlohmann/json.hpp>

namespace fts {

Config::Config(const std::filesystem::path &pathJsonFile) {
  std::ifstream jsonFile(pathJsonFile);
  nlohmann::json json_ = nlohmann::json::parse(jsonFile, nullptr, false);

  if (json_.is_discarded()) {
    throw ConfigurationException(
        "Incorrect configuration format. Need json file");
  }

  ngram_min_length = json_["ngram_min_length"].get<size_t>();
  ngram_max_length = json_["ngram_max_length"].get<size_t>();

  if (ngram_max_length < ngram_min_length) {
    throw ConfigurationException(
        "Incorrect ngram size. Max length can`t be less than min length");
  }

  stop_words = json_["stop_words"].get<std::vector<std::string>>();
}

static void split_string(std::string const &str, const char delim,
                         std::vector<std::string> &out) {
  size_t start = 0;
  size_t end = 0;

  while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
    end = str.find(delim, start);
    out.push_back(str.substr(start, end - start));
  }
}

std::vector<ParsedString> parse(std::string text, const Config &config) {
  std::vector<ParsedString> parsed_ngrams;

  //удаление пунктуации
  auto it = std::remove_if(text.begin(), text.end(), ::ispunct);
  text.erase(it, text.end());

  //приведение к нижнему регистру
  std::transform(text.begin(), text.end(), text.begin(), tolower);

  //разбиение строки на слова
  std::vector<std::string> splited_string;
  split_string(text, ' ', splited_string);

  //удаление стоп-слов
  long i = 0;
  for (const auto &word : splited_string) {
    for (const auto &stop_word : config.getStopWords()) {
      if (word == stop_word) {
        splited_string.erase(splited_string.begin() + i);
      }
    }
    i++;
  }

  //деление на термы
  for (size_t i = 0; i < splited_string.size(); ++i) {
    ParsedString current_word;
    for (size_t j = config.getNgramMinLength(); j <= config.getNgramMaxLength();
         ++j) {
      if (splited_string[i].length() < j) {
        break;
      }
      current_word.word_ngrams.push_back(splited_string[i].substr(0, j));
    }
    if (!current_word.word_ngrams.empty()) {
      current_word.word_position = i;
      parsed_ngrams.push_back(current_word);
    }
  }
  return parsed_ngrams;
}

} // namespace fts
