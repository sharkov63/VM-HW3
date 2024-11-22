#include "Analyzer.h"
#include "ByteFile.h"
#include "Error.h"
#include "Parser.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#define AUX_REACHED (1 << 0)
#define AUX_RIFT (1 << 1)
#define AUX_LONG (1 << 2)

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
static const uint8_t *codeBegin;
static const uint8_t *codeEnd;
static const char *strtab;

static void setFile(ByteFile &&byteFile) {
  file = std::move(byteFile);
  codeBegin = file.getCode();
  codeEnd = codeBegin + file.getCodeSizeBytes() - 1;
  strtab = file.getStringTable();
}

static std::unique_ptr<uint8_t[]> aux;
static size_t instructionNum;

static inline uint8_t *auxpOf(const uint8_t *ip) {
  return &aux[ip - codeBegin];
}

static inline int32_t getInstLength(const uint8_t *ip) {
  const uint8_t *auxp = auxpOf(ip);
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

template <int N>
static inline int32_t getIdiomLength(const uint8_t *ip) {
  const uint8_t *idiomp = ip;
  for (int i = 0; i < N; ++i)
    ip += getInstLength(ip);
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

  template <int N>
  void dump() const;
};

template <int N>
void IdiomOccurrence::dump() const {
  std::cerr << fmt::format("{} occurrences of\n", count);
  dumpIdiom<N>(idiomp);
  std::cerr << '\n';
}

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

  const std::vector<IdiomOccurrence> &getIdioms() const { return idioms; }

private:
  void collect();
  void collectAt(const uint8_t *ip);

  void count();

  void report();

private:
  std::vector<IdiomOccurrence> idioms;
};

template <int N>
void IdiomAnalyzer<N>::analyze() {
  collect();
  count();
  // report();
}

template <int N>
void IdiomAnalyzer<N>::collect() {
  const uint8_t *ip = codeBegin;
  while (ip < codeEnd) {
    collectAt(ip);
    ip += getInstLength(ip);
  }
}

template <int N>
void IdiomAnalyzer<N>::collectAt(const uint8_t *ip) {
  if (!(*auxpOf(ip) & AUX_REACHED))
    return;
  const uint8_t *idiomp = ip;
  for (int i = 0; i < N; ++i) {
    if (ip >= codeEnd)
      return;
    if (i > 0 && (*auxpOf(ip) & AUX_RIFT))
      return;
    ip += getInstLength(ip);
  }
  idioms.push_back({idiomp});
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
  std::reverse(idioms.begin(), idioms.end());
  idioms.shrink_to_fit();
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

namespace {

class Parser {
public:
  void parse();

private:
  void parseAt(const uint8_t *ip);

  /// \return nextIp
  const uint8_t *invokeParser(const uint8_t *ip, const uint8_t **label);

  void enqueue(const uint8_t *ip);

  std::vector<const uint8_t *> stack;
};

} // namespace

void Parser::parse() {
  stack.reserve(sizeof(int32_t) * file.getCodeSizeBytes());
  instructionNum = 0;
  aux = std::unique_ptr<uint8_t[]>(new uint8_t[file.getCodeSizeBytes()]);
  memset(aux.get(), 0, file.getCodeSizeBytes());
  for (int i = 0; i < file.getPublicSymbolNum(); ++i) {
    int32_t offset = file.getPublicSymbolOffset(i);
    const uint8_t *beginIp = file.getAddressFor(offset);
    enqueue(beginIp);
  }
  while (!stack.empty()) {
    const uint8_t *ip = stack.back();
    stack.pop_back();
    try {
      parseAt(ip);
    } catch (std::runtime_error &e) {
      runtimeError("parsing fail at {:#x}: {}", ip - codeBegin, e.what());
    }
  }
}

void Parser::parseAt(const uint8_t *ip) {
  if (ip < codeBegin || ip >= codeEnd)
    runtimeError("reached out of bytecode bounds");
  uint8_t code = *ip;
  uint8_t *auxp = auxpOf(ip);
  ++instructionNum;
  const uint8_t *label = nullptr; // optional
  const uint8_t *nextIp = invokeParser(ip, &label);
  int32_t length = nextIp - ip;
  if (length > 1) {
    assert(length >= 1 + sizeof(int32_t));
    *auxp |= AUX_LONG;
    memcpy(auxp + 1, &length, sizeof(int32_t));
  }
  if (code == I_END)
    return;
  if (nextIp >= codeEnd)
    runtimeError("unexpected end of the bytecode after {:#x}", ip - codeBegin);
  if (label) {
    *auxpOf(label) |= AUX_RIFT;
    enqueue(label);
  }
  switch (code) {
  case I_BEGIN:
  case I_BEGINcl: {
    *auxp |= AUX_RIFT;
    break;
  }
  case I_JMP: {
    // We don't move to next instruction on unconditional jump
    return;
  }
  case I_CJMPz:
  case I_CJMPnz:
  case I_CALLC:
  case I_CALL: {
    *auxpOf(nextIp) |= AUX_RIFT;
    break;
  }
  }
  enqueue(nextIp);
}

const uint8_t *Parser::invokeParser(const uint8_t *ip, const uint8_t **label) {
#undef PARSER_ON_LABEL
#define PARSER_ON_LABEL(l) *label = file.getAddressFor(l);

  PARSE_INST;

#undef PARSER_ON_LABEL
#define PARSER_ON_LABEL(l) ;
  return ip;
}

void Parser::enqueue(const uint8_t *ip) {
  uint8_t *auxp = auxpOf(ip);
  if (*auxp & AUX_REACHED)
    return;
  *auxp |= AUX_REACHED;
  stack.push_back(ip);
}

void lama::analyze(ByteFile &&byteFile) {
  setFile(std::move(byteFile));
  if (file.getCodeSizeBytes() == 0) {
    runtimeError("empty bytefile");
  }
  {
    Parser parser;
    parser.parse();
  }
  IdiomAnalyzer<1> analyzer1;
  analyzer1.analyze();
  auto idioms1 = analyzer1.getIdioms();
  IdiomAnalyzer<2> analyzer2;
  analyzer2.analyze();
  auto idioms2 = analyzer2.getIdioms();
  std::cerr << fmt::format("Found {} idioms\n\n",
                           idioms1.size() + idioms2.size());
  int pos1 = 0;
  int pos2 = 0;
  while (pos1 < idioms1.size() || pos2 < idioms2.size()) {
    if (pos2 >= idioms2.size() || idioms1[pos1].count >= idioms2[pos2].count) {
      idioms1[pos1++].dump<1>();
    } else {
      idioms2[pos2++].dump<2>();
    }
  }
}
