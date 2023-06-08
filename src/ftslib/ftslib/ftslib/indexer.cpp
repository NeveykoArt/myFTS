#include <cstring>
#include <fstream>
#include <ftslib/indexer.hpp>
#include <ftslib/parser.hpp>
#include <picosha2.h>

namespace fts {

// IndexBuilder

void IndexBuilder::addDocument(size_t document_id,
                               const std::string &name_of_doc,
                               const Config &config) {
  if (index_.getDocs().find(document_id) == index_.getDocs().end()) {
    index_.getDocs()[document_id] = name_of_doc;
    const std::vector<ParsedString> parsed_text = parse(name_of_doc, config);
    for (const auto &word : parsed_text) {
      for (const auto &term : word.word_ngrams) {
        index_.getEntries()[term][document_id].push_back(word.word_position);
      }
    }
  }
}

// TextIndexWrite

void TextIndexWriter::write(const std::filesystem::path &path_of_doc,
                            Index &index) {
  std::filesystem::create_directories(path_of_doc / "text");
  std::filesystem::create_directories(path_of_doc / "text/docs");
  for (const auto &[doc_id, doc] : index.getDocs()) {
    std::ofstream file(path_of_doc / "text/docs" / std::to_string(doc_id));
    file << doc;
  }

  std::filesystem::create_directories(path_of_doc / "text/entries");
  for (auto &[term, entry] : index.getEntries()) {
    std::string hash_hex_term;
    picosha2::hash256_hex_string(term, hash_hex_term);
    std::ofstream file(path_of_doc / "text/entries" /
                       hash_hex_term.substr(0, 6));

    file << term + ' ' + std::to_string(entry.size());
    for (const auto &[doc_id, position] : entry) {
      file << ' ' + std::to_string(doc_id) + ' ' +
                  std::to_string(position.size());
      for (const auto &pos : position) {
        file << ' ' + std::to_string(pos);
      }
    }
  }
}

// BinaryIndexWrite

static void writeHeader(BinaryBuffer &bin_buf) {
  const std::uint8_t section_count = 3;
  const std::uint32_t section_offset = 3;
  bin_buf.write(&section_count, sizeof(section_count));
  std::vector<std::string> section_name = {"dictionary", "entries", "docs"};

  for (const auto &section : section_name) {
    const std::uint8_t section_size = section.size() + 1;

    bin_buf.write(&section_size, sizeof(section_size));
    bin_buf.write(section.data(), section_size - 1);
    bin_buf.write(&section_offset, sizeof(section_offset));
  }
}

static std::unordered_map<size_t, std::uint32_t>
writeDocs(BinaryBuffer &bin_buf, Index &index) {
  std::unordered_map<size_t, std::uint32_t> doc_offset;
  const std::uint32_t docs_size = index.getDocs().size();
  bin_buf.write(&docs_size, sizeof(docs_size));

  for (const auto &[docs_id, docs] : index.getDocs()) {
    doc_offset[docs_id] = bin_buf.size();
    const std::uint8_t doc_size = docs.size() + 1;
    bin_buf.write(&doc_size, sizeof(doc_size));
    bin_buf.write(docs.data(), doc_size - 1);
  }
  return doc_offset;
}

static void writeDictionary(
    BinaryBuffer &bin_buf, Index &index,
    std::unordered_map<std::string, std::uint32_t> &entry_offset) {
  Trie trie;
  for (const auto &[term, entry] : index.getEntries()) {
    trie.add(term, 0);
  }
  trie.serialize(bin_buf, entry_offset);
}

static std::unordered_map<std::string, std::uint32_t>
writeEntries(BinaryBuffer &bin_buf, Index &index,
             std::unordered_map<size_t, std::uint32_t> &doc_offset) {
  std::unordered_map<std::string, std::uint32_t> entry_offset;
  for (auto &[term, entry] : index.getEntries()) {
    entry_offset[term] = bin_buf.size();

    const std::uint32_t doc_count = entry.size();
    bin_buf.write(&doc_count, sizeof(doc_count));

    for (const auto &[doc_id, position] : entry) {

      const std::uint32_t pos_count = position.size();
      bin_buf.write(&doc_offset[doc_id], sizeof(doc_offset[doc_id]));
      bin_buf.write(&pos_count, sizeof(pos_count));

      for (const auto &pos : position) {
        const std::uint32_t position = pos;
        bin_buf.write(&position, sizeof(position));
      }
    }
  }
  return entry_offset;
}

void BinaryIndexWriter::write(const std::filesystem::path &path_of_doc,
                              Index &index) {
  std::filesystem::create_directories(path_of_doc / "binary");
  std::ofstream binfile(path_of_doc / "binary/binary", std::ios_base::binary);

  BinaryBuffer header_buf;
  BinaryBuffer dictionary_buf;
  BinaryBuffer docs_buf;
  BinaryBuffer entries_buf;

  writeHeader(header_buf);
  auto doc_offset = writeDocs(docs_buf, index);
  auto entry_offset = writeEntries(entries_buf, index, doc_offset);
  writeDictionary(dictionary_buf, index, entry_offset);

  const std::uint32_t dictionary_offset = header_buf.size();
  header_buf.writeTo(&dictionary_offset, sizeof(dictionary_offset),
                     static_cast<size_t>(12));

  const std::uint32_t entries_offset =
      dictionary_offset + dictionary_buf.size();
  header_buf.writeTo(&entries_offset, sizeof(entries_offset),
                     static_cast<size_t>(24));

  const std::uint32_t docs_offset = entries_offset + entries_buf.size();
  header_buf.writeTo(&docs_offset, sizeof(docs_offset),
                     static_cast<size_t>(33));

  binfile.write(header_buf.data().data(),
                static_cast<std::streamsize>(header_buf.size()));
  binfile.write(dictionary_buf.data().data(),
                static_cast<std::streamsize>(dictionary_buf.size()));
  binfile.write(entries_buf.data().data(),
                static_cast<std::streamsize>(entries_buf.size()));
  binfile.write(docs_buf.data().data(),
                static_cast<std::streamsize>(docs_buf.size()));
}

// BinaryBuffer

void BinaryBuffer::write(const void *src, size_t size) {
  std::size_t prev_size = binary_data.size();
  binary_data.resize(binary_data.size() + size);
  std::memcpy(binary_data.data() + prev_size, src, size);
}

void BinaryBuffer::writeTo(const void *src, size_t size, size_t offset) {
  const auto *data_start_address = static_cast<const char *>(src);
  auto offset_iter = binary_data.begin();
  std::advance(offset_iter, offset);
  std::copy(data_start_address, data_start_address + size, offset_iter);
}

// Trie

void Trie::add(const std::string &word, std::uint32_t entry_offset) {
  TrieNode *next_p = root.get();
  for (const auto &ch : word) {
    const auto search_it = next_p->children_node.find(ch);
    if (search_it == next_p->children_node.end()) {
      next_p->children_node[ch] = std::make_unique<TrieNode>();
    }
    next_p = next_p->children_node[ch].get();
  }
  next_p->is_leaf = 1;
  next_p->entry_offset = entry_offset;
}

void Trie::serialize(BinaryBuffer &bin_buf,
                     const std::unordered_map<std::string, std::uint32_t>
                         &entry_offset) const {
  std::string term;
  serialize_(root.get(), bin_buf, entry_offset, term);
}

std::uint32_t Trie::serialize_(
    const TrieNode *next_p, BinaryBuffer &bin_buf,
    const std::unordered_map<std::string, std::uint32_t> &entry_offset,
    std::string &term) const {
  const std::uint32_t start_pos_write = bin_buf.size();
  const std::uint32_t childs_count = next_p->children_node.size();
  const std::uint32_t zero = 0xffffffff;
  const std::uint8_t is_leaf = next_p->is_leaf;
  std::size_t start_child_offset = 0;

  bin_buf.write(&childs_count, sizeof(childs_count));
  for (const auto &[key, trie_node] : next_p->children_node) {
    bin_buf.write(&key, sizeof(key));
  }
  start_child_offset = bin_buf.size();
  for (std::uint32_t i = 0; i < childs_count; ++i) {
    bin_buf.write(&zero, sizeof(zero));
  }
  bin_buf.write(&is_leaf, sizeof(is_leaf));
  if (is_leaf == 1) {
    const std::uint32_t offset = entry_offset.at(term);
    bin_buf.write(&offset, sizeof(offset));
  }
  for (const auto &[key, trie_node] : next_p->children_node) {
    term.push_back(key);
    const std::uint32_t child_offset =
        serialize_(trie_node.get(), bin_buf, entry_offset, term);
    bin_buf.writeTo(&child_offset, sizeof(child_offset), start_child_offset);
    start_child_offset += sizeof(child_offset);
  }
  if (!std::empty(term)) {
    term.pop_back();
  }
  return start_pos_write;
}

} // namespace fts
