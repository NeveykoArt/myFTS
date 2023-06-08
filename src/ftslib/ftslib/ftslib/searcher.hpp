#pragma once

#include <filesystem>
#include <ftslib/parser.hpp>
#include <map>
#include <unordered_map>
#include <vector>

namespace fts {

class IndexAccessor {
public:
  virtual std::string loadDocument(size_t identifier) const = 0;
  virtual bool totalDocs(double &file_count) const = 0;
  virtual std::vector<size_t> getDocByTerm(const std::string &term) const = 0;
  virtual size_t getCountTermsInDoc(const std::string &term,
                                    size_t identifier) const = 0;
};

class TextIndexAccessor : public IndexAccessor {
private:
  std::filesystem::path path_of_docs;

public:
  explicit TextIndexAccessor(std::filesystem::path new_path)
      : path_of_docs(std::move(new_path)) {}
  std::string loadDocument(size_t identifier) const override;
  bool totalDocs(double &file_count) const override;
  std::vector<size_t> getDocByTerm(const std::string &term) const override;
  size_t getCountTermsInDoc(const std::string &term,
                            size_t identifier) const override;
};

class Header {
private:
  std::uint8_t section_count;
  std::unordered_map<std::string, std::uint32_t> sections;

public:
  explicit Header(const char *data);
  const std::uint32_t &sectionOffset(const std::string &name) const {
    const auto offset = sections.find(name);
    return offset->second;
  }
};

class DocumentAccessor {
private:
  const char *document_data;

public:
  explicit DocumentAccessor(const char *d) : document_data(d) {}
  std::string loadDocument(size_t identifier) const;
  size_t totalDocs() const;
};

class DictionaryAccessor {
private:
  const char *dictionary_data;

public:
  explicit DictionaryAccessor(const char *d) : dictionary_data(d) {}
  std::uint32_t retrieve(const std::string &word);
};

class EntryAccessor {
private:
  const char *entry_data;

public:
  explicit EntryAccessor(const char *d) : entry_data(d) {}
  std::map<size_t, std::vector<size_t>>
  getTermInfos(std::uint32_t entry_offset);
};

class BinaryIndexAccessor : public IndexAccessor {
private:
  const char *binary_index_data;
  Header header;

public:
  explicit BinaryIndexAccessor(const char *d, Header &h)
      : binary_index_data(d), header(h) {}
  std::string loadDocument(size_t identifier) const override;
  bool totalDocs(double &file_count) const override;
  std::vector<size_t> getDocByTerm(const std::string &term) const override;
  size_t getCountTermsInDoc(const std::string &term,
                            size_t identifier) const override;
};

class BinaryReader {
private:
  const char *binary_data;
  const char *current_data;

public:
  explicit BinaryReader(const char *buf)
      : binary_data(buf), current_data(buf) {}
  void readBinary(void *dest, size_t size);
  const char *current() const { return current_data; }
  void move(size_t size) { current_data += size; }
  void moveBack() { current_data = binary_data; }
};

struct Result {
  size_t document_id;
  double score;
  std::string name_of_doc;
};

std::vector<Result> search(const Config &config,
                           const fts::IndexAccessor &index,
                           const std::string &query);

void printResult(const std::vector<Result> &result);

std::string getStringSearchResult(const std::vector<Result> &search_result);

const char *mmap_bin_file(const std::filesystem::path &file_path);

void parseTextEntry(
    const std::filesystem::path &path_of_doc,
    std::map<std::string, std::map<size_t, std::vector<size_t>>> &entry);

void parseBinaryEntry(
    const std::filesystem::path &path_of_doc,
    const std::string &term,
    std::map<size_t, std::vector<size_t>> &entry);
} // namespace fts
