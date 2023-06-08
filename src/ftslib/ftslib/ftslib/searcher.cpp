#include <algorithm>
#include <cmath>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <ftslib/indexer.hpp>
#include <ftslib/searcher.hpp>
#include <iostream>
#include <picosha2.h>
#include <sys/mman.h>

namespace fts {

// Searcher

static void sort_by_score(std::vector<Result> &search_result) {
  std::sort(search_result.begin(), search_result.end(),
            [](const auto &lhs, const auto &rhs) {
              return lhs.score != rhs.score ? lhs.score > rhs.score
                                            : lhs.document_id < rhs.document_id;
            });
}

std::vector<Result> search(const Config &config,
                           const fts::IndexAccessor &index,
                           const std::string &query) {
  const auto parsed_query = parse(query, config);
  std::map<size_t, double> result;
  double N = 0.0;
  if (!index.totalDocs(N)) {
    throw ConfigurationException(
        "There no files in directory you choose. Forgot index.");
  }
  for (const auto &word : parsed_query) {
    for (const auto &term : word.word_ngrams) {
      std::vector<size_t> docs;

      docs = index.getDocByTerm(term);

      const auto df = static_cast<double>(docs.size());
      for (const auto &identifier : docs) {
        const auto tf =
            static_cast<double>(index.getCountTermsInDoc(term, identifier));
        result[identifier] += tf * log(N / df);
      }
    }
  }
  std::vector<Result> results;

  for (const auto &[document_id, score] : result) {
    const auto text = index.loadDocument(document_id);
    results.push_back({document_id, score, text});
  }
  sort_by_score(results);
  return results;
}

void printResult(const std::vector<Result> &result) {
  std::cout << "\tSearch result:\n";
  std::cout << "\tTop\tId\tScore\t\tText\n";
  size_t i = 1;
  for (const auto &[id, score, text] : result) {
    std::cout << "\t" << i << "\t" << id << "\t" << score << "\t" << text
              << "\n";
    ++i;
    if (i == 20) {
      break;
    }
  }
}

std::string getStringSearchResult(const std::vector<Result> &search_result) {
  std::string result;
  size_t i = 1;
  result += "\tSearch result:\n";
  result += "\tTop\tId\tScore\t\tText\n";
  for (const auto &[document_id, score, text] : search_result) {
    result += ("\t" + std::to_string(i) + "\t" + std::to_string(document_id) +
               "\t" + std::to_string(score) + "\t" + text + "\n");
    ++i;
    if (i == 20) {
      break;
    }
  }
  return result;
}

// parseEntry

void parseTextEntry(
    const std::filesystem::path &path_of_doc,
    std::map<std::string, std::map<size_t, std::vector<size_t>>> &entry) {
  std::fstream file(path_of_doc, std::fstream::in);

  std::string term;
  file >> term;

  size_t doccount = 0;
  file >> doccount;

  std::map<size_t, std::vector<size_t>> countindoc;
  for (size_t i = 0; i < doccount; ++i) {
    size_t document_id = 0;
    file >> document_id;

    size_t poscount = 0;
    file >> poscount;

    countindoc[document_id].push_back(poscount);
    for (size_t j = 0; j < poscount; ++j) {
      size_t pos = 0;
      file >> pos;
      countindoc[document_id].push_back(pos);
    }
  }
  entry[term] = countindoc;
}

void parseBinaryEntry(
    const std::filesystem::path &path_of_doc,
    const std::string &term,
    std::map<size_t, std::vector<size_t>> &entry) {

  const auto *index_data = fts::mmap_bin_file(path_of_doc);
  fts::Header header(index_data);
  BinaryReader reader(index_data);
  reader.move(header.sectionOffset("dictionary"));
  DictionaryAccessor dictionary(reader.current());
  const auto entry_offset = dictionary.retrieve(term);
  reader.moveBack();
  reader.move(header.sectionOffset("entries"));
  EntryAccessor entries(reader.current());
  std::map<size_t, std::vector<size_t>> buf = entries.getTermInfos(entry_offset);
  for (auto &[id, pos] : buf) {
    entry[id].push_back(pos.size());
    for (auto p : pos) {
      entry[id].push_back(p);
    }
  }
}

// TextIndexAccessor

std::string TextIndexAccessor::loadDocument(size_t identifier) const {
  std::string document;
  std::ifstream file(path_of_docs / "docs" / std::to_string(identifier));
  std::getline(file, document);
  return document;
}

bool TextIndexAccessor::totalDocs(double &file_count) const {
  const std::string path = path_of_docs.string() + "/docs";
  if (!std::filesystem::exists(path)) {
    return false;
  }
  DIR *dirp = opendir(path.c_str());
  struct dirent *entry = nullptr;

  while ((entry = readdir(dirp)) != nullptr) {
    if (entry->d_type == DT_REG) {
      file_count++;
    }
  }
  closedir(dirp);
  if (file_count == 0) {
    return false;
  }
  return true;
}

std::vector<size_t>
TextIndexAccessor::getDocByTerm(const std::string &term) const {
  std::vector<size_t> docs;
  std::map<std::string, std::map<size_t, std::vector<size_t>>> entries;
  std::string hash_hex_term;
  picosha2::hash256_hex_string(term, hash_hex_term);
  parseTextEntry(path_of_docs / "entries" / hash_hex_term.substr(0, 6), entries);

  for (const auto &[docs_id, position] : entries[term]) {
    docs.push_back(docs_id);
  }

  return docs;
}

size_t TextIndexAccessor::getCountTermsInDoc(const std::string &term,
                                             size_t identifier) const {
  std::map<std::string, std::map<size_t, std::vector<size_t>>> entries;
  std::string hash_hex_term;
  picosha2::hash256_hex_string(term, hash_hex_term);
  parseTextEntry(path_of_docs / "entries" / hash_hex_term.substr(0, 6), entries);

  return entries[term][identifier].size();
}

// Header

Header::Header(const char *data) {
  BinaryReader reader(data);
  reader.readBinary(&section_count, sizeof(section_count));
  for (std::size_t i = 0; i < section_count; ++i) {
    std::uint8_t length = 0;
    reader.readBinary(&length, sizeof(length));
    std::string name(length - 1, ' ');
    reader.readBinary(name.data(), name.length());
    std::uint32_t section_offset = 0;
    reader.readBinary(&section_offset, sizeof(section_offset));
    sections[name] = section_offset;
  }
}

// DocumentAccessor

std::string DocumentAccessor::loadDocument(size_t identifier) const {
  BinaryReader reader(document_data);
  reader.move(identifier);
  std::uint8_t length = 0;
  reader.readBinary(&length, sizeof(length));
  std::string document(length - 1, ' ');
  reader.readBinary(document.data(), document.length());
  return document;
}

size_t DocumentAccessor::totalDocs() const {
  BinaryReader reader(document_data);
  std::uint32_t titles_count = 0;
  reader.readBinary(&titles_count, sizeof(titles_count));
  return titles_count;
}

// DictionaryAccessor

std::uint32_t DictionaryAccessor::retrieve(const std::string &word) {
  BinaryReader reader(dictionary_data);
  std::uint32_t children_count = 0;
  std::uint8_t is_leaf = 0;
  std::uint32_t entry_offset = 0;
  for (const auto &symbol : word) {
    std::uint32_t child_offset = 0;
    reader.readBinary(&children_count, sizeof(children_count));
    std::size_t child_pos = children_count;
    for (std::size_t i = 0; i < children_count; ++i) {
      std::int8_t letter = 0;
      reader.readBinary(&letter, sizeof(letter));
      if (letter == symbol) {
        child_pos = i;
      }
    }
    reader.move(sizeof(child_offset) * child_pos);
    reader.readBinary(&child_offset, sizeof(child_offset));
    reader.move(sizeof(child_offset) * (children_count - child_pos - 1));
    reader.readBinary(&is_leaf, sizeof(is_leaf));
    if (is_leaf == 1) {
      reader.readBinary(&entry_offset, sizeof(entry_offset));
    }
    if (child_offset != 0) {
      reader.moveBack();
      reader.move(child_offset);
    }
  }
  reader.readBinary(&children_count, sizeof(children_count));
  reader.move(static_cast<size_t>(children_count * 5));
  reader.readBinary(&is_leaf, sizeof(is_leaf));
  reader.readBinary(&entry_offset, sizeof(entry_offset));
  return entry_offset;
}

// EntryAccessor

std::map<size_t, std::vector<size_t>>
EntryAccessor::getTermInfos(std::uint32_t entry_offset) {
  BinaryReader reader(entry_data);
  std::map<size_t, std::vector<size_t>> term_infos;
  reader.move(entry_offset);
  std::uint32_t doc_count = 0;
  reader.readBinary(&doc_count, sizeof(doc_count));
  for (size_t i = 0; i < doc_count; ++i) {
    std::uint32_t doc_offset = 0;
    reader.readBinary(&doc_offset, sizeof(doc_offset));
    std::uint32_t pos_count = 0;
    reader.readBinary(&pos_count, sizeof(pos_count));
    std::vector<size_t> positions;
    for (size_t j = 0; j < pos_count; ++j) {
      std::uint32_t pos = 0;
      reader.readBinary(&pos, sizeof(pos));
      positions.push_back(pos);
    }
    term_infos[doc_offset] = positions;
  }
  return term_infos;
}

// BinaryReader

void BinaryReader::readBinary(void *dest, size_t size) {
  std::memcpy(dest, current_data, size);
  current_data += size;
}

// BinaryIndexAccessor

std::string BinaryIndexAccessor::loadDocument(size_t identifier) const {
  BinaryReader reader(binary_index_data);
  reader.move(header.sectionOffset("docs"));
  DocumentAccessor doc(reader.current());
  return doc.loadDocument(identifier);
}

bool BinaryIndexAccessor::totalDocs(double &file_count) const {
  BinaryReader reader(binary_index_data);
  reader.move(header.sectionOffset("docs"));
  DocumentAccessor doc(reader.current());
  file_count = static_cast<double>(doc.totalDocs());
  if (file_count == 0.0) {
    return false;
  }
  return true;
}

std::vector<size_t>
BinaryIndexAccessor::getDocByTerm(const std::string &term) const {
  BinaryReader reader(binary_index_data);
  reader.move(header.sectionOffset("dictionary"));
  DictionaryAccessor dictionary(reader.current());
  const auto entry_offset = dictionary.retrieve(term);
  reader.moveBack();
  reader.move(header.sectionOffset("entries"));
  EntryAccessor entry(reader.current());
  const auto term_infos = entry.getTermInfos(entry_offset);
  std::vector<size_t> docs;
  for (const auto &[offset, positions] : term_infos) {
    docs.push_back(offset);
  }
  return docs;
}

size_t BinaryIndexAccessor::getCountTermsInDoc(const std::string &term,
                                               size_t identifier) const {
  BinaryReader reader(binary_index_data);
  reader.move(header.sectionOffset("dictionary"));
  DictionaryAccessor dictionary(reader.current());
  const auto entry_offset = dictionary.retrieve(term);
  reader.moveBack();
  reader.move(header.sectionOffset("entries"));
  EntryAccessor entry(reader.current());
  auto term_infos = entry.getTermInfos(entry_offset);
  return term_infos[identifier].size();
}

// mmap_bin_file

const char *mmap_bin_file(const std::filesystem::path &file_path) {
  int file = open((file_path).c_str(), O_CLOEXEC);
  if (file == -1) {
        perror("Can`t open binfile");
        exit(255);
  }
  size_t size = std::filesystem::file_size(file_path);
  const char *src = nullptr;
  src =
      static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file, 0));
  return src;
}

} // namespace fts
