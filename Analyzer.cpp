#include "Analyzer.h"
#include "ByteFile.h"
#include "Error.h"
#include "Inst.h"
#include <algorithm>
#include <iostream>

#ifndef MAX_INSTS
#define MAX_INSTS (1 << 20)
#endif

using namespace lama;

static size_t getParamNum(uint8_t code, const uint8_t *paramBegin) {
  switch (code) {
  case I_BINOP_Add:
  case I_BINOP_Sub:
  case I_BINOP_Mul:
  case I_BINOP_Div:
  case I_BINOP_Mod:
  case I_BINOP_Lt:
  case I_BINOP_Leq:
  case I_BINOP_Gt:
  case I_BINOP_Geq:
  case I_BINOP_Eq:
  case I_BINOP_Neq:
  case I_BINOP_And:
  case I_BINOP_Or:
  case I_STA:
  case I_END:
  case I_DROP:
  case I_DUP:
  case I_ELEM:
  case I_PATT_StrCmp:
  case I_PATT_String:
  case I_PATT_Array:
  case I_PATT_Sexp:
  case I_PATT_Boxed:
  case I_PATT_UnBoxed:
  case I_PATT_Closure:
  case I_CALL_Lread:
  case I_CALL_Lwrite:
  case I_CALL_Llength:
  case I_CALL_Lstring:
    return 0;
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
    runtimeError("I_CLOSURE is not supported yet");
  default:
    runtimeError("unsupported instruction code {:#04x}", code);
  }
}

static ByteFile file;
const uint8_t *codeBegin;
const uint8_t *codeEnd;

static void setFile(ByteFile &&byteFile) {
  file = std::move(byteFile);
  codeBegin = reinterpret_cast<const uint8_t *>(file.getCode());
  codeEnd = codeBegin + file.getCodeSizeBytes() - 1;
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
  const uint8_t *nextIp = ip + 1 + sizeof(int32_t) * paramNum;
  if (nextIp > codeEnd) {
    runtimeError(
        "unexpected end of bytefile: instruction {:#x} requires {} parameters");
  }
  InstRef ref(ip, paramNum);
  InstSeq::append(ref);
  return nextIp;
}

static void buildInstSeq() {
  const uint8_t *ip = codeBegin;
  while (ip < codeEnd) {
    try {
      ip = buildInstSeqStep(ip);
    } catch (std::runtime_error &e) {
      runtimeError("runtime error at {:#x}: {}", ip - codeBegin, e.what());
    }
  }
}

// static void markRifts() {
//   for (size_t instIndex = 0; instIndex < InstSeq::getLength(); ++instIndex) {
//   }
// }

void lama::analyze(ByteFile &&byteFile) {
  reset();
  setFile(std::move(byteFile));
  if (file.getCodeSizeBytes() == 0) {
    runtimeError("empty bytefile");
  }
  buildInstSeq();
  // markRifts();
}
