#include "Analyzer.h"
#include "ByteFile.h"
#include "Error.h"
#include "Inst.h"
#include "Parser.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>

#ifndef MAX_INSTS
#define MAX_INSTS (1 << 20)
#endif

#ifndef MAX_IDIOMS
#define MAX_IDIOMS (1 << 20)
#endif

#define AUX_RIFT (1 << 0)
#define AUX_LONG (1 << 1)

using namespace lama;

// static size_t getParamNum(uint8_t code, const uint8_t *paramBegin) {
//   switch (code) {
//   case I_BINOP_Add:
//   case I_BINOP_Sub:
//   case I_BINOP_Mul:
//   case I_BINOP_Div:
//   case I_BINOP_Mod:
//   case I_BINOP_Lt:
//   case I_BINOP_Leq:
//   case I_BINOP_Gt:
//   case I_BINOP_Geq:
//   case I_BINOP_Eq:
//   case I_BINOP_Neq:
//   case I_BINOP_And:
//   case I_BINOP_Or:
//   case I_STA:
//   case I_END:
//   case I_DROP:
//   case I_DUP:
//   case I_ELEM:
//   case I_PATT_StrCmp:
//   case I_PATT_String:
//   case I_PATT_Array:
//   case I_PATT_Sexp:
//   case I_PATT_Boxed:
//   case I_PATT_UnBoxed:
//   case I_PATT_Closure:
//   case I_CALL_Lread:
//   case I_CALL_Lwrite:
//   case I_CALL_Llength:
//   case I_CALL_Lstring:
//     return 0;
//   case I_CONST:
//   case I_STRING:
//   case I_JMP:
//   case I_LD_Global:
//   case I_LD_Local:
//   case I_LD_Arg:
//   case I_LD_Access:
//   case I_LDA_Global:
//   case I_LDA_Local:
//   case I_LDA_Arg:
//   case I_LDA_Access:
//   case I_ST_Global:
//   case I_ST_Local:
//   case I_ST_Arg:
//   case I_ST_Access:
//   case I_CJMPz:
//   case I_CJMPnz:
//   case I_CALLC:
//   case I_ARRAY:
//   case I_LINE:
//   case I_CALL_Barray:
//     return 1;
//   case I_SEXP:
//   case I_BEGIN:
//   case I_BEGINcl:
//   case I_CALL:
//   case I_TAG:
//   case I_FAIL:
//     return 2;
//   case I_CLOSURE:
//     runtimeError("I_CLOSURE is not supported yet");
//   default:
//     runtimeError("unsupported instruction code {:#04x}", code);
//   }
// }

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

// namespace {

// class InstSeq {
// public:
//   struct Element {
//     InstRef ref;
//     bool rift = false;
//   };

//   static void reset();

//   static void append(InstRef ref);

//   static Element &at(size_t index) { return data[index]; }
//   static size_t getLength() { return length; }

//   static size_t search(const uint8_t *ip);

// private:
//   static std::array<Element, MAX_INSTS> data;
//   static size_t length;
// };

// } // namespace

// std::array<InstSeq::Element, MAX_INSTS> InstSeq::data;
// size_t InstSeq::length;

// void InstSeq::reset() { length = 0; }

// void InstSeq::append(InstRef ref) {
//   if (length >= MAX_INSTS)
//     runtimeError("exhausted max instructions limit of {}", MAX_INSTS);
//   data[length++] = {ref};
// }

// size_t InstSeq::search(const uint8_t *ip) {
//   return std::lower_bound(
//              data.begin(), data.begin() + length, ip,
//              [](const Element &element, const uint8_t *ip) -> bool {
//                return element.ref.getIp() < ip;
//              }) -
//          data.begin();
// }

// namespace {

// template <int N>
// struct IdiomOccurence {
//   InstIdiom<N> idiom;
//   size_t count;

//   bool operator<(const IdiomOccurence &other) const {
//     return std::tie(count, idiom) < std::tie(other.count, other.idiom);
//   }
//   bool operator==(const IdiomOccurence &other) const {
//     return std::tie(count, idiom) == std::tie(other.count, other.idiom);
//   }
// };

// template <int N>
// class IdiomCollector {
// public:
//   void reset();

//   void collect();

//   void count();

//   void report(std::ostream &os);

// private:
//   void append(InstIdiom<N> idiom);

//   static std::optional<InstIdiom<N>> idiomAt(size_t instIndex);

// private:
//   std::array<InstIdiom<N>, MAX_IDIOMS> data;
//   size_t length = 0;

//   std::array<IdiomOccurence<N>, MAX_IDIOMS> occurenceData;
//   size_t occurenceDataLength;
// };

// } // namespace

// template <int N>
// void IdiomCollector<N>::reset() {
//   length = 0;
//   occurenceDataLength = 0;
// }

// template <int N>
// void IdiomCollector<N>::collect() {
//   for (size_t instIndex = 0; instIndex < InstSeq::getLength() - N + 1;
//        ++instIndex) {
//     auto idiom = idiomAt(instIndex);
//     if (!idiom)
//       continue;
//     append(*idiom);
//   }
// }

// template <int N>
// void IdiomCollector<N>::count() {
//   std::sort(data.begin(), data.begin() + length);
//   for (size_t index = 0; index < length;) {
//     size_t nextIndex = index + 1;
//     while (nextIndex < length && data[nextIndex] == data[index])
//       ++nextIndex;
//     size_t count = nextIndex - index;
//     occurenceData[occurenceDataLength++] = {data[index], count};
//     index = nextIndex;
//   }
//   std::sort(occurenceData.begin(), occurenceData.begin() +
//   occurenceDataLength);
// }

// template <int N>
// void IdiomCollector<N>::report(std::ostream &os) {
//   os << fmt::format("Found {} {}-idioms\n", occurenceDataLength, N);
//   for (int index = occurenceDataLength - 1; index >= 0; --index) {
//     auto &idiomOccurence = occurenceData[index];
//     auto &idiom = idiomOccurence.idiom;
//     idiom.dump(os);
//     os << fmt::format(" x{}\n", idiomOccurence.count);
//   }
// }

// template <int N>
// void IdiomCollector<N>::append(InstIdiom<N> idiom) {
//   if (length >= MAX_IDIOMS)
//     runtimeError("exhausted limit of {}-idioms ({})", N, MAX_IDIOMS);
//   data[length++] = idiom;
// }

// template <int N>
// std::optional<InstIdiom<N>> IdiomCollector<N>::idiomAt(size_t instIndex) {
//   InstIdiom<N> result;
//   result.at(0) = InstSeq::at(instIndex).ref;
//   for (size_t pos = 1; pos < N; ++pos) {
//     auto &element = InstSeq::at(instIndex + pos);
//     if (element.rift)
//       return std::nullopt;
//     result.at(pos) = element.ref;
//   }
//   return result;
// }

// static IdiomCollector<1> idioms1;
// static IdiomCollector<2> idioms2;

// static void reset() {
//   InstSeq::reset();
//   idioms1.reset();
//   idioms2.reset();
// }

// /// \return pointer to next instruction
// static const uint8_t *buildInstSeqStep(const uint8_t *ip) {
//   uint8_t code = *ip;
//   size_t paramNum = getParamNum(code, ip + 1);
//   const uint8_t *nextIp = ip + 1 + sizeof(int32_t) * paramNum;
//   if (nextIp > codeEnd) {
//     runtimeError(
//         "unexpected end of bytefile: instruction {:#x} requires {}
//         parameters");
//   }
//   InstRef ref(ip, paramNum);
//   InstSeq::append(ref);
//   return nextIp;
// }

// static const uint8_t *buildInstSeqStep2(const uint8_t *ip) {
//   PARSE_INST;
//   return ip;
// }

// static void buildInstSeq() {
//   const uint8_t *ip = codeBegin;
//   while (ip < codeEnd) {
//     try {
//       const uint8_t *nextip = buildInstSeqStep2(ip);
//       InstRef ref(ip, nextip - ip);
//       InstSeq::append(ref);
//       ip = nextip;
//     } catch (std::runtime_error &e) {
//       runtimeError("runtime error at {:#x}: {}", ip - codeBegin, e.what());
//     }
//   }
// }

// static void markRiftAt(const uint8_t *ip) {
//   size_t instIndex = InstSeq::search(ip);
//   if (instIndex >= InstSeq::getLength() ||
//       InstSeq::at(instIndex).ref.getIp() != ip) {
//     runtimeError("invalid instruction address of {:#x}", ip - codeBegin);
//   }
//   InstSeq::at(instIndex).rift = true;
// }

// static void markRifts() {
//   InstSeq::at(0).rift = true;
//   for (size_t instIndex = 0; instIndex < InstSeq::getLength(); ++instIndex) {
//     auto &inst = InstSeq::at(instIndex);
//     uint8_t code = inst.ref.getCode();
//     switch (code) {
//     case I_BEGIN:
//     case I_BEGINcl: {
//       inst.rift = true;
//       break;
//     }
//     case I_JMP:
//     case I_CJMPz:
//     case I_CJMPnz:
//     case I_CALL: {
//       int32_t offset;
//       memcpy(&offset, inst.ref.getIp() + 1, sizeof(offset));
//       markRiftAt(file.getAddressFor(offset));
//       break;
//     }
//     default:
//       break;
//     }
//   }
// }

// NEW IMPL

// class InstRef2 {
// public:
//   InstRef2() : ip(nullptr) {}
//   explicit InstRef2(const uint8_t *ip) : ip(ip) {}

//   const uint8_t *getIp() const { return ip; }
//   int32_t getLength() const;

//   bool operator<(InstRef2 other) const;
//   bool operator==(InstRef2 other) const;

// protected:
//   const uint8_t *ip;
// };

// int32_t InstRef2::getLength() const {
// }

// bool InstRef2::operator<(InstRef2 other) const {
//   int32_t length = getLength(), otherLength = other.getLength();
//   if (length != otherLength)
//     return length < otherLength;
//   return memcmp(ip, other.ip, length) < 0;
// }

// bool InstRef2::operator==(InstRef2 other) const {
//   int32_t length = getLength(), otherLength = other.getLength();
//   if (length != otherLength)
//     return false;
//   return memcmp(ip, other.ip, length) == 0;
// }

// template <int N>
// class Idiom {
// public:
//   InstRef2 &at(size_t i) { return elements[i]; }
//   const InstRef2 &at(size_t i) const { return elements[i]; }

//   bool operator<(InstIdiom<N> other) const { return elements <
//   other.elements; } bool operator==(InstIdiom<N> other) const {
//     return elements == other.elements;
//   }

// private:
//   std::array<InstRef2, N> elements;
// };

// template <int N>
// class IdiomCollector {
// public:
//   IdiomCollector() { idioms.reserve(instructionNum + 1); }

//   void collect();

// private:
//   void collectAt(const uint8_t *ip);

// private:
//   std::vector<Idiom<N>> idioms;
// };

// template <int N>
// void IdiomCollector<N>::collectAt(const uint8_t *base) {
//   const uint8_t *ip = base;
//   Idiom<N> idiom;
//   for (int i = 0; i < N; ++i) {
//     if (i > 0 && (*getAuxP(ip) & AUX_RIFT))
//       return;
//     idiom[i] = InstRef2(ip);
//     ip += idiom[i].getLength();
//   }
//   idioms.push_back(idiom);
// }

// template <int N>
// void IdiomCollector<N>::collect() {
//   const uint8_t *ip = codeBegin;
//   while (ip < codeEnd) {
//     collectAt(ip);
//     ip += InstRef2(ip).getLength();
//   }
// }

// NEW IMPL 2

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
  void report();

private:
  std::vector<IdiomOccurrence> idioms;
};

template <int N>
void IdiomAnalyzer<N>::analyze() {
  collect();
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
