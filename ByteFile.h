#pragma once

#include <memory>

namespace lama {

class ByteFile {
public:
  ByteFile() = default;
  ByteFile(std::unique_ptr<const uint8_t[]> data, size_t sizeBytes);

  static ByteFile load(std::string path);

  const uint8_t *getCode() const { return code; }
  size_t getCodeSizeBytes() { return codeSizeBytes; }

  const uint8_t *getAddressFor(size_t offset) const;

  const char *getStringTable() const {
    return reinterpret_cast<const char *>(stringTable);
  }
  const uint8_t *getStringAt(size_t offset) const;

  size_t getPublicSymbolNum() const { return publicSymbolsNum; }

  int32_t getPublicSymbolOffset(size_t index) const {
    return publicSymbolTable[2 * index + 1];
  }

private:
  void init();

private:
  std::unique_ptr<const uint8_t[]> data;
  size_t sizeBytes;

  const uint8_t *stringTable;
  size_t stringTableSizeBytes;

  const int32_t *publicSymbolTable;
  size_t publicSymbolsNum;

  const uint8_t *code;
  size_t codeSizeBytes;
};

} // namespace lama
