#include "Inst.h"
#include "fmt/format.h"
#include <cassert>
#include <cstring>
#include <ostream>

using namespace lama;

// int32_t InstRef::getParam(size_t index) const {
//   assert(index < paramNum);
//   int32_t result;
//   memcpy(&result, data + 1, sizeof(int32_t));
//   return result;
// }

bool InstRef::operator<(InstRef other) const {
  if (length != other.length)
    return length < other.length;
  return memcmp(data, other.data, length) < 0;
}

bool InstRef::operator==(InstRef other) const {
  if (length != other.length)
    return false;
  return memcmp(data, other.data, length) == 0;
}

// static void dumpInstCode(uint8_t code, std::ostream &out) {
//   switch (code) {
//   case I_BINOP_Add: {
//     out << "BINOP +";
//     return;
//   }
//   case I_BINOP_Sub: {
//     out << "BINOP -";
//     return;
//   }
//   case I_BINOP_Mul: {
//     out << "BINOP *";
//     return;
//   }
//   case I_BINOP_Div: {
//     out << "BINOP /";
//     return;
//   }
//   case I_BINOP_Mod: {
//     out << "BINOP %";
//     return;
//   }
//   case I_BINOP_Lt: {
//     out << "BINOP <";
//     return;
//   }
//   case I_BINOP_Leq: {
//     out << "BINOP <=";
//     return;
//   }
//   case I_BINOP_Gt: {
//     out << "BINOP >";
//     return;
//   }
//   case I_BINOP_Geq: {
//     out << "BINOP >=";
//     return;
//   }
//   case I_BINOP_Eq: {
//     out << "BINOP ==";
//     return;
//   }
//   case I_BINOP_Neq: {
//     out << "BINOP !=";
//     return;
//   }
//   case I_BINOP_And: {
//     out << "BINOP &&";
//     return;
//   }
//   case I_BINOP_Or: {
//     out << "BINOP ||";
//     return;
//   }

//   case I_CONST: {
//     out << "CONST";
//     return;
//   }
//   case I_STRING: {
//     out << "STRING";
//     return;
//   }
//   case I_SEXP: {
//     out << "SEXP";
//     return;
//   }
//   case I_STA: {
//     out << "STA";
//     return;
//   }
//   case I_JMP: {
//     out << "JMP";
//     return;
//   }
//   case I_END: {
//     out << "END";
//     return;
//   }
//   case I_DROP: {
//     out << "DROP";
//     return;
//   }
//   case I_DUP: {
//     out << "DUP";
//     return;
//   }
//   case I_ELEM: {
//     out << "ELEM";
//     return;
//   }

//   case I_LD_Global: {
//     out << "LD G";
//     return;
//   }
//   case I_LD_Local: {
//     out << "LD L";
//     return;
//   }
//   case I_LD_Arg: {
//     out << "LD A";
//     return;
//   }
//   case I_LD_Access: {
//     out << "LD C";
//     return;
//   }

//   case I_LDA_Global: {
//     out << "LDA G";
//     return;
//   }
//   case I_LDA_Local: {
//     out << "LDA L";
//     return;
//   }
//   case I_LDA_Arg: {
//     out << "LDA A";
//     return;
//   }
//   case I_LDA_Access: {
//     out << "LDA C";
//     return;
//   }

//   case I_ST_Global: {
//     out << "ST G";
//     return;
//   }
//   case I_ST_Local: {
//     out << "ST L";
//     return;
//   }
//   case I_ST_Arg: {
//     out << "ST A";
//     return;
//   }
//   case I_ST_Access: {
//     out << "ST C";
//     return;
//   }

//   case I_CJMPz: {
//     out << "CJMPz";
//     return;
//   }
//   case I_CJMPnz: {
//     out << "CJMPnz";
//     return;
//   }
//   case I_BEGIN: {
//     out << "BEGIN";
//     return;
//   }
//   case I_BEGINcl: {
//     out << "CBEGIN";
//     return;
//   }
//   case I_CLOSURE: {
//     out << "CLOSURE";
//     return;
//   }
//   case I_CALLC: {
//     out << "CALLC";
//     return;
//   }
//   case I_CALL: {
//     out << "CALL";
//     return;
//   }
//   case I_TAG: {
//     out << "TAG";
//     return;
//   }
//   case I_ARRAY: {
//     out << "ARRAY";
//     return;
//   }
//   case I_FAIL: {
//     out << "FAIL";
//     return;
//   }
//   case I_LINE: {
//     out << "LINE";
//     return;
//   }

//   case I_PATT_StrCmp: {
//     out << "PATT =str";
//     return;
//   }
//   case I_PATT_String: {
//     out << "PATT #string";
//     return;
//   }
//   case I_PATT_Array: {
//     out << "PATT #array";
//     return;
//   }
//   case I_PATT_Sexp: {
//     out << "PATT #sexp";
//     return;
//   }
//   case I_PATT_Boxed: {
//     out << "PATT #ref";
//     return;
//   }
//   case I_PATT_UnBoxed: {
//     out << "PATT #val";
//     return;
//   }
//   case I_PATT_Closure: {
//     out << "PATT #fun";
//     return;
//   }

//   case I_CALL_Lread: {
//     out << "CALL Lread";
//     return;
//   }
//   case I_CALL_Lwrite: {
//     out << "CALL Lwrite";
//     return;
//   }
//   case I_CALL_Llength: {
//     out << "CALL Llength";
//     return;
//   }
//   case I_CALL_Lstring: {
//     out << "CALL Lstring";
//     return;
//   }
//   case I_CALL_Barray: {
//     out << "CALL Barray";
//     return;
//   }
//   }
//   out << "UNKNOWN";
// }

void InstRef::dump(std::ostream &out) const {
  // dumpInstCode(getCode(), out);
  // for (int i = 0; i < paramNum; ++i)
  //   out << fmt::format(" {:#x}", getParam(i));
}
