#include "Analyzer.h"
#include "ByteFile.h"
#include "Error.h"
#include "Parser.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>

#define AUX_RIFT (1 << 0)
#define AUX_LONG (1 << 1)

using namespace lama;

namespace {

enum InstCode {
  I_BINOP_Add = 0x01,
  I_BINOP_Sub = 0x02,
  I_BINOP_Mul = 0x03,
  I_BINOP_Div = 0x04,
  I_BINOP_Mod = 0x05,
  I_BINOP_Lt = 0x06,
  I_BINOP_Leq = 0x07,
  I_BINOP_Gt = 0x08,
  I_BINOP_Geq = 0x09,
  I_BINOP_Eq = 0x0a,
  I_BINOP_Neq = 0x0b,
  I_BINOP_And = 0x0c,
  I_BINOP_Or = 0x0d,

  I_CONST = 0x10,
  I_STRING = 0x11,
  I_SEXP = 0x12,
  I_STA = 0x14,
  I_JMP = 0x15,
  I_END = 0x16,
  I_DROP = 0x18,
  I_DUP = 0x19,
  I_ELEM = 0x1b,

  I_LD_Global = 0x20,
  I_LD_Local = 0x21,
  I_LD_Arg = 0x22,
  I_LD_Access = 0x23,

  I_LDA_Global = 0x30,
  I_LDA_Local = 0x31,
  I_LDA_Arg = 0x32,
  I_LDA_Access = 0x33,

  I_ST_Global = 0x40,
  I_ST_Local = 0x41,
  I_ST_Arg = 0x42,
  I_ST_Access = 0x43,

  I_CJMPz = 0x50,
  I_CJMPnz = 0x51,
  I_BEGIN = 0x52,
  I_BEGINcl = 0x53,
  I_CLOSURE = 0x54,
  I_CALLC = 0x55,
  I_CALL = 0x56,
  I_TAG = 0x57,
  I_ARRAY = 0x58,
  I_FAIL = 0x59,
  I_LINE = 0x5a,

  I_PATT_StrCmp = 0x60,
  I_PATT_String = 0x61,
  I_PATT_Array = 0x62,
  I_PATT_Sexp = 0x63,
  I_PATT_Boxed = 0x64,
  I_PATT_UnBoxed = 0x65,
  I_PATT_Closure = 0x66,

  I_CALL_Lread = 0x70,
  I_CALL_Lwrite = 0x71,
  I_CALL_Llength = 0x72,
  I_CALL_Lstring = 0x73,
  I_CALL_Barray = 0x74,
};

}

static ByteFile file;
const uint8_t *codeBegin;
const uint8_t *codeEnd;
const char *strtab;
std::unique_ptr<uint8_t[]> aux;
size_t instructionNum;

static void setFile(ByteFile &&byteFile) {
  file = std::move(byteFile);
  codeBegin = file.getCode();
  codeEnd = codeBegin + file.getCodeSizeBytes() - 1;
  strtab = file.getStringTable();
}

static inline uint8_t *getAuxP(const uint8_t *ip) {
  return &aux[ip - codeBegin];
}

static inline int32_t getInstLength(const uint8_t *ip) {
  const uint8_t *auxp = getAuxP(ip);
  if (!(*auxp & AUX_LONG))
    return 1;
  int32_t length;
  memcpy(&length, auxp + 1, sizeof(int32_t));
  return length;
}

static void dumpInst(const uint8_t *ip) {
#undef PARSER_PRINTF_INST
#define PARSER_PRINTF_INST(...) fprintf(stderr, __VA_ARGS__);

  PARSE_INST;

#undef PARSER_PRINTF_INST
#define PARSER_PRINTF_INST(...) ;
}

#define IDIOM_FAIL (-1)

/// \return IDIOM_FAIL if not a valid idiom
template <int N>
static inline int32_t getIdiomLength(const uint8_t *ip) {
  const uint8_t *idiomp = ip;
  for (int i = 0; i < N; ++i) {
    if (ip >= codeEnd)
      return IDIOM_FAIL;
    if (i > 0 && (*getAuxP(ip) & AUX_RIFT))
      return IDIOM_FAIL;
    ip += getInstLength(ip);
  }
  return ip - idiomp;
}

template <int N>
static void dumpIdiom(const uint8_t *ip) {
  for (int i = 0; i < N; ++i) {
    dumpInst(ip);
    std::cerr << '\n';
    ip += getInstLength(ip);
  }
}

struct IdiomOccurrence {
  const uint8_t *idiomp;
  int32_t count;
};

template <int N>
static inline bool idiomCmp(const IdiomOccurrence &lhs,
                            const IdiomOccurrence &rhs) {
  int32_t lhsLength = getIdiomLength<N>(lhs.idiomp);
  int32_t rhsLength = getIdiomLength<N>(rhs.idiomp);
  if (lhsLength != rhsLength)
    return lhsLength < rhsLength;
  return memcmp(lhs.idiomp, rhs.idiomp, lhsLength) < 0;
}

template <int N>
static inline bool idiomEq(const IdiomOccurrence &lhs,
                           const IdiomOccurrence &rhs) {
  int32_t lhsLength = getIdiomLength<N>(lhs.idiomp);
  int32_t rhsLength = getIdiomLength<N>(rhs.idiomp);
  if (lhsLength != rhsLength)
    return false;
  return memcmp(lhs.idiomp, rhs.idiomp, lhsLength) == 0;
}

static inline bool countCmp(const IdiomOccurrence &lhs,
                            const IdiomOccurrence &rhs) {
  return lhs.count < rhs.count;
}

template <int N>
class IdiomAnalyzer {
public:
  IdiomAnalyzer() { idioms.reserve(instructionNum); }

  void analyze();

  void collect();
  void count();
  void report();

private:
  std::vector<IdiomOccurrence> idioms;
};

template <int N>
void IdiomAnalyzer<N>::analyze() {
  collect();
  count();
  report();
}

template <int N>
void IdiomAnalyzer<N>::collect() {
  const uint8_t *ip = codeBegin;
  while (ip < codeEnd) {
    if (getIdiomLength<N>(ip) > 0)
      idioms.push_back({ip});
    ip += getInstLength(ip);
  }
}

template <int N>
void IdiomAnalyzer<N>::count() {
  std::sort(idioms.begin(), idioms.end(), idiomCmp<N>);
  int nextSlot = 0;
  int i = 0;
  while (i < idioms.size()) {
    int j = i + 1;
    while (j < idioms.size() && idiomEq<N>(idioms[i], idioms[j]))
      ++j;
    idioms[nextSlot++] = {idioms[i].idiomp, j - i};
    i = j;
  }
  idioms.resize(nextSlot);
  std::sort(idioms.begin(), idioms.end(), countCmp);
}

template <int N>
void IdiomAnalyzer<N>::report() {
  std::cerr << fmt::format("Found {} {}-idioms\n\n", idioms.size(), N);
  for (int index = idioms.size() - 1; index >= 0; --index) {
    std::cerr << fmt::format("{} occurrences of\n", idioms[index].count);
    dumpIdiom<N>(idioms[index].idiomp);
    std::cerr << '\n';
  }
}

static void reset() { aux.release(); }

static const uint8_t *prepareAuxStep(const uint8_t *ip) {

#undef PARSER_ON_LABEL
#define PARSER_ON_LABEL(offset) aux[offset] |= AUX_RIFT;

  PARSE_INST;

#undef PARSER_ON_LABEL
#define PARSER_ON_LABEL ;

  return ip;
}

static void parse() {
  instructionNum = 0;
  aux = std::unique_ptr<uint8_t[]>(new uint8_t[file.getCodeSizeBytes()]);
  memset(aux.get(), 0, file.getCodeSizeBytes());
  aux[0] |= AUX_RIFT;

  const uint8_t *ip = codeBegin;
  while (ip < codeEnd) {
    ++instructionNum;
    uint8_t *auxp = getAuxP(ip);
    if (*ip == I_BEGIN || *ip == I_BEGINcl)
      *auxp |= AUX_RIFT;
    const uint8_t *iend = prepareAuxStep(ip);
    int32_t length = iend - ip;
    if (length > 1) {
      assert(length >= 1 + sizeof(int32_t));
      *auxp |= AUX_LONG;
      memcpy(auxp + 1, &length, sizeof(int32_t));
    }
    ip = iend;
  }
}

void lama::analyze(ByteFile &&byteFile) {
  reset();
  setFile(std::move(byteFile));
  if (file.getCodeSizeBytes() == 0) {
    runtimeError("empty bytefile");
  }
  parse();
  {
    IdiomAnalyzer<1> analyzer;
    analyzer.analyze();
  }
  {
    IdiomAnalyzer<2> analyzer;
    analyzer.analyze();
  }
}
