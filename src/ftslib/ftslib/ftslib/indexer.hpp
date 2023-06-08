#pragma once

#include <filesystem>
#include <ftslib/parser.hpp>
#include <map>
#include <unordered_map>
#include <vector>

namespace fts {

class Index {
private:
  std::map<size_t, std::string> docs;
  std::map<std::string, std::map<size_t, std::vector<size_t>>> entries;

public:
  explicit Index() = default;
  std::map<size_t, std::string> &getDocs() { return docs; }
  std::map<std::string, std::map<size_t, std::vector<size_t>>> &getEntries() {
    return entries;
  }
};

class IndexBuilder {
private:
  Index index_;

public:
  explicit IndexBuilder() { Index index_; };
  void addDocument(size_t document_id, const std::string &name_of_doc,
                   const Config &config);
  Index &getIndex() { return index_; }
};

class BinaryBuffer {
private:
  std::vector<char> binary_data;

public:
  void write(const void *src, size_t size);
  void writeTo(const void *src, size_t size, size_t offset);
  std::vector<char> &data() { return binary_data; };
  std::size_t size() const { return binary_data.size(); };
};

class IndexWriter {
public:
  virtual void write(const std::filesystem::path &path_of_doc,
                     Index &index) = 0;
};

class TextIndexWriter : public IndexWriter {
public:
  void write(const std::filesystem::path &path_of_doc, Index &index) override;
};

class BinaryIndexWriter : public IndexWriter {
public:
  void write(const std::filesystem::path &path_of_doc, Index &index) override;
};

struct TrieNode {
  std::map<char, std::unique_ptr<TrieNode>> children_node;
  std::uint32_t entry_offset = 0;
  std::uint8_t is_leaf = 0;
};

class Trie {
private:
  std::unique_ptr<TrieNode> root;

  std::uint32_t serialize_(
      const TrieNode *next_p, BinaryBuffer &bin_buf,
      const std::unordered_map<std::string, std::uint32_t> &entry_offset,
      std::string &term) const;

public:
  explicit Trie() : root(std::make_unique<TrieNode>()){};
  void add(const std::string &word, std::uint32_t entry_offset);
  void serialize(BinaryBuffer &bin_buf,
                 const std::unordered_map<std::string, std::uint32_t>
                     &entry_offset) const;
};

} // namespace fts
