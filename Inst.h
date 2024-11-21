#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <ostream>

namespace lama {

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

// class InstRef2 {
// public:
//   InstRef2() : data(nullptr) {}
//   explicit InstRef2(const uint8_t *data, size_t length)
//       : data(data), length(length) {}

// protected:
//   const uint8_t *data;
//   size_t length;
// };

class InstRef {
public:
  InstRef() : data(nullptr) {}
  explicit InstRef(const uint8_t *inst, size_t length)
      : data(inst), length(length) {}

  const uint8_t *getIp() const { return data; }
  size_t getLength() const { return length; }
  uint8_t getCode() const { return *data; }

  bool operator<(InstRef other) const;
  bool operator==(InstRef other) const;

  void dump(std::ostream &out) const;

protected:
  const uint8_t *data;
  size_t length;
};

template <int N>
class InstIdiom {
public:
  InstRef &at(size_t i) { return elements[i]; }
  const InstRef &at(size_t i) const { return elements[i]; }

  bool operator<(InstIdiom<N> other) const { return elements < other.elements; }
  bool operator==(InstIdiom<N> other) const {
    return elements == other.elements;
  }

  void dump(std::ostream &out) const {
    for (size_t pos = 0; pos < N; ++pos) {
      at(pos).dump(out);
      if (pos < N - 1)
        out << "; ";
    }
  }

private:
  std::array<InstRef, N> elements;
};

} // namespace lama
