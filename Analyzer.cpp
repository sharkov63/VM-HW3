#include "Analyzer.h"
#include "ByteFile.h"
#include "Error.h"
#include "Inst.h"
#include "Parser.h"
#include <algorithm>
#include <iostream>
#include <optional>

#ifndef MAX_INSTS
#define MAX_INSTS (1 << 20)
#endif

#ifndef MAX_IDIOMS
#define MAX_IDIOMS (1 << 20)
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
const char *strtab;

static void setFile(ByteFile &&byteFile) {
  file = std::move(byteFile);
  codeBegin = file.getCode();
  codeEnd = codeBegin + file.getCodeSizeBytes() - 1;
  strtab = file.getStringTable();
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

  static size_t search(const uint8_t *ip);

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

size_t InstSeq::search(const uint8_t *ip) {
  return std::lower_bound(
             data.begin(), data.begin() + length, ip,
             [](const Element &element, const uint8_t *ip) -> bool {
               return element.ref.getIp() < ip;
             }) -
         data.begin();
}

namespace {

template <int N>
struct IdiomOccurence {
  InstIdiom<N> idiom;
  size_t count;

  bool operator<(const IdiomOccurence &other) const {
    return std::tie(count, idiom) < std::tie(other.count, other.idiom);
  }
  bool operator==(const IdiomOccurence &other) const {
    return std::tie(count, idiom) == std::tie(other.count, other.idiom);
  }
};

template <int N>
class IdiomCollector {
public:
  void reset();

  void collect();

  void count();

  void report(std::ostream &os);

private:
  void append(InstIdiom<N> idiom);

  static std::optional<InstIdiom<N>> idiomAt(size_t instIndex);

private:
  std::array<InstIdiom<N>, MAX_IDIOMS> data;
  size_t length = 0;

  std::array<IdiomOccurence<N>, MAX_IDIOMS> occurenceData;
  size_t occurenceDataLength;
};

} // namespace

template <int N>
void IdiomCollector<N>::reset() {
  length = 0;
  occurenceDataLength = 0;
}

template <int N>
void IdiomCollector<N>::collect() {
  for (size_t instIndex = 0; instIndex < InstSeq::getLength() - N + 1;
       ++instIndex) {
    auto idiom = idiomAt(instIndex);
    if (!idiom)
      continue;
    append(*idiom);
  }
}

template <int N>
void IdiomCollector<N>::count() {
  std::sort(data.begin(), data.begin() + length);
  for (size_t index = 0; index < length;) {
    size_t nextIndex = index + 1;
    while (nextIndex < length && data[nextIndex] == data[index])
      ++nextIndex;
    size_t count = nextIndex - index;
    occurenceData[occurenceDataLength++] = {data[index], count};
    index = nextIndex;
  }
  std::sort(occurenceData.begin(), occurenceData.begin() + occurenceDataLength);
}

template <int N>
void IdiomCollector<N>::report(std::ostream &os) {
  os << fmt::format("Found {} {}-idioms\n", occurenceDataLength, N);
  for (int index = occurenceDataLength - 1; index >= 0; --index) {
    auto &idiomOccurence = occurenceData[index];
    auto &idiom = idiomOccurence.idiom;
    idiom.dump(os);
    os << fmt::format(" x{}\n", idiomOccurence.count);
  }
}

template <int N>
void IdiomCollector<N>::append(InstIdiom<N> idiom) {
  if (length >= MAX_IDIOMS)
    runtimeError("exhausted limit of {}-idioms ({})", N, MAX_IDIOMS);
  data[length++] = idiom;
}

template <int N>
std::optional<InstIdiom<N>> IdiomCollector<N>::idiomAt(size_t instIndex) {
  InstIdiom<N> result;
  result.at(0) = InstSeq::at(instIndex).ref;
  for (size_t pos = 1; pos < N; ++pos) {
    auto &element = InstSeq::at(instIndex + pos);
    if (element.rift)
      return std::nullopt;
    result.at(pos) = element.ref;
  }
  return result;
}

static IdiomCollector<1> idioms1;
static IdiomCollector<2> idioms2;

static void reset() {
  InstSeq::reset();
  idioms1.reset();
  idioms2.reset();
}

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

static const uint8_t *buildInstSeqStep2(const uint8_t *ip) {
  PARSE_INST;
  return ip;
}

static void buildInstSeq() {
  const uint8_t *ip = codeBegin;
  while (ip < codeEnd) {
    try {
      const uint8_t *nextip = buildInstSeqStep2(ip);
      InstRef ref(ip, nextip - ip);
      InstSeq::append(ref);
      ip = nextip;
    } catch (std::runtime_error &e) {
      runtimeError("runtime error at {:#x}: {}", ip - codeBegin, e.what());
    }
  }
}

static void markRiftAt(const uint8_t *ip) {
  size_t instIndex = InstSeq::search(ip);
  if (instIndex >= InstSeq::getLength() ||
      InstSeq::at(instIndex).ref.getIp() != ip) {
    runtimeError("invalid instruction address of {:#x}", ip - codeBegin);
  }
  InstSeq::at(instIndex).rift = true;
}

static void markRifts() {
  InstSeq::at(0).rift = true;
  for (size_t instIndex = 0; instIndex < InstSeq::getLength(); ++instIndex) {
    auto &inst = InstSeq::at(instIndex);
    uint8_t code = inst.ref.getCode();
    switch (code) {
    case I_BEGIN:
    case I_BEGINcl: {
      inst.rift = true;
      break;
    }
    case I_JMP:
    case I_CJMPz:
    case I_CJMPnz:
    case I_CALL: {
      int32_t offset;
      memcpy(&offset, inst.ref.getIp() + 1, sizeof(offset));
      markRiftAt(file.getAddressFor(offset));
      break;
    }
    default:
      break;
    }
  }
}

void lama::analyze(ByteFile &&byteFile) {
  reset();
  setFile(std::move(byteFile));
  if (file.getCodeSizeBytes() == 0) {
    runtimeError("empty bytefile");
  }
  buildInstSeq();
  markRifts();
  idioms1.collect();
  idioms2.collect();
  idioms1.count();
  idioms2.count();
  idioms1.report(std::cerr);
  idioms2.report(std::cerr);
}
