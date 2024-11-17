#include "Analyzer.h"
#include "ByteFile.h"
#include "Error.h"
#include "Inst.h"
#include <algorithm>

#ifndef MAX_INSTS
#define MAX_INSTS (1 << 20)
#endif

using namespace lama;

static size_t getParamNum(uint8_t code, const uint8_t *paramBegin) {
  switch (code) {
  case I_CONST:
  case I_STRING:
  case I_JMP:
  case I_LD_Global:
  case I_LD_Local:
  case I_LD_Arg:
  case I_LD_Access:
  case I_LDA_Global:
  case I_LDA_Local:
  case I_LDA_Arg:
  case I_LDA_Access:
  case I_ST_Global:
  case I_ST_Local:
  case I_ST_Arg:
  case I_ST_Access:
  case I_CJMPz:
  case I_CJMPnz:
  case I_CALLC:
  case I_ARRAY:
  case I_LINE:
  case I_CALL_Barray:
    return 1;
  case I_SEXP:
  case I_BEGIN:
  case I_BEGINcl:
  case I_CALL:
  case I_TAG:
  case I_FAIL:
    return 2;
  case I_CLOSURE:
    runtimeError("not supported");
  default:
    return 0;
  }
}

static ByteFile file;
const uint8_t *codeBegin;
const uint8_t *codeEnd;

static void setFile(ByteFile &&byteFile) {
  file = std::move(byteFile);
  codeBegin = reinterpret_cast<const uint8_t *>(file.getCode());
  codeEnd = codeBegin + file.getCodeSizeBytes();
}

namespace {

class InstSeq {
public:
  struct Element {
    InstRef ref;
    bool rift = false;
  };

  static void reset();

  static void append(InstRef ref);

  static Element &at(size_t index) { return data[index]; }
  static size_t getLength() { return length; }

  static Element &search(const uint8_t *ip);

private:
  static std::array<Element, MAX_INSTS> data;
  static size_t length;
};

} // namespace

std::array<InstSeq::Element, MAX_INSTS> InstSeq::data;
size_t InstSeq::length;

void InstSeq::reset() { length = 0; }

void InstSeq::append(InstRef ref) {
  if (length >= MAX_INSTS)
    runtimeError("exhausted max instructions limit of {}", MAX_INSTS);
  data[length++] = {ref};
}

InstSeq::Element &InstSeq::search(const uint8_t *ip) {
  return *std::lower_bound(
      data.begin(), data.begin() + length, ip,
      [](const Element &element, const uint8_t *ip) -> bool {
        return element.ref.getIp() < ip;
      });
}

static void reset() { InstSeq::reset(); }

/// \return pointer to next instruction
static const uint8_t *buildInstSeqStep(const uint8_t *ip) {
  uint8_t code = *ip;
  size_t paramNum = getParamNum(code, ip + 1);
  InstRef ref(ip, paramNum);
  InstSeq::append(ref);
  return ip + 1 + sizeof(int32_t) * paramNum;
}

static void buildInstSeq() {
  const uint8_t *ip = codeBegin;
  while (ip < codeEnd)
    ip = buildInstSeqStep(ip);
}

// static void markRifts() {
//   for (size_t instIndex = 0; instIndex < InstSeq::getLength(); ++instIndex) {

//   }
// }

void lama::analyze(ByteFile &&byteFile) {
  reset();
  if (byteFile.getCodeSizeBytes() == 0) {
    // TODO
  }
  setFile(std::move(byteFile));
  buildInstSeq();
  // markRifts();
}
