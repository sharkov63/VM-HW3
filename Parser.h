#pragma once

namespace lama::parser {

inline const char *ops[] = {
    "+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
inline const char *pats[] = {"=str", "#string", "#array", "#sexp",
                             "#ref", "#val",    "#fun"};
inline const char *lds[] = {"LD", "LDA", "ST"};

#define _PARSER_NEXT_BYTE(b)                                                   \
  if (ip >= codeEnd)                                                           \
    PARSER_FAIL("unexpected reach out of code region bounds");                 \
  uint8_t b = *ip;                                                             \
  PARSER_ON_BYTE(b);                                                           \
  ++ip;

#define _PARSER_NEXT_INT(n)                                                    \
  if (ip + sizeof(int32_t) >= codeEnd)                                         \
    PARSER_FAIL("expected an int, but reached out of code region bounds");     \
  int32_t n;                                                                   \
  memcpy(&n, ip, sizeof(int32_t));                                             \
  PARSER_ON_INT(n);                                                            \
  ip += sizeof(int32_t);

#define _PARSER_NEXT_STRING(s)                                                 \
  if (ip + sizeof(int32_t) >= codeEnd)                                         \
    PARSER_FAIL("expected a word, but reached out of code region bounds");     \
  int32_t s##offset;                                                           \
  memcpy(&s##offset, ip, sizeof(int32_t));                                     \
  const char *s = &strtab[s##offset];                                          \
  PARSER_ON_STRING(s);                                                         \
  ip += sizeof(int32_t);

#define _PARSER_NEXT_LABEL(l)                                                  \
  if (ip + sizeof(int32_t) >= codeEnd)                                         \
    PARSER_FAIL("expected a word, but reached out of code region bounds");     \
  int32_t l;                                                                   \
  memcpy(&l, ip, sizeof(int32_t));                                             \
  PARSER_ON_LABEL(l);                                                          \
  ip += sizeof(int32_t);

#define _PARSER_FAIL_UNSUPPORTED_INST                                          \
  PARSER_FAIL(fmt::format("unsupported instruction code {:#04x}", x));

#define PARSER_FAIL(msg) runtimeError(msg);
#define PARSER_ON_INSTCODE(c) ;
#define PARSER_ON_BYTE(b) ;
#define PARSER_ON_INT(n) ;
#define PARSER_ON_STRING(s) ;
#define PARSER_ON_LABEL(l) ;
#define PARSER_PRINTF_INST(...) ;

/// Generic Lama instruction parser.
/// Source: byterun.c
///
/// \pre ip is mutable instruction pointer
/// \pre codeEnd is the end of code region, pointer to 0xff byte.
/// \pre strtab is the pointer to the string table
#define PARSE_INST                                                             \
  using namespace ::lama::parser;                                              \
  uint8_t x = *ip;                                                             \
  uint8_t h = (x & 0xF0) >> 4;                                                 \
  uint8_t l = x & 0x0F;                                                        \
  PARSER_ON_INSTCODE(x);                                                       \
  ip++;                                                                        \
                                                                               \
  switch (h) {                                                                 \
  /* BINOP */                                                                  \
  case 0: {                                                                    \
    PARSER_PRINTF_INST("BINOP\t%s", ops[l - 1]);                               \
    break;                                                                     \
  }                                                                            \
  case 1: {                                                                    \
    switch (l) {                                                               \
    case 0: {                                                                  \
      _PARSER_NEXT_INT(n);                                                     \
      PARSER_PRINTF_INST("CONST\t%d", n);                                      \
      break;                                                                   \
    }                                                                          \
    case 1: {                                                                  \
      _PARSER_NEXT_STRING(s);                                                  \
      PARSER_PRINTF_INST("STRING\t%s", s);                                     \
      break;                                                                   \
    }                                                                          \
    case 2: {                                                                  \
      _PARSER_NEXT_STRING(s);                                                  \
      _PARSER_NEXT_INT(n);                                                     \
      PARSER_PRINTF_INST("SEXP\t%s %d", s, n);                                 \
      break;                                                                   \
    }                                                                          \
    case 3: {                                                                  \
      PARSER_PRINTF_INST("STI");                                               \
      break;                                                                   \
    }                                                                          \
    case 4: {                                                                  \
      PARSER_PRINTF_INST("STA");                                               \
      break;                                                                   \
    }                                                                          \
    case 5: {                                                                  \
      _PARSER_NEXT_LABEL(l);                                                   \
      PARSER_PRINTF_INST("JMP\t0x%.8x", l);                                    \
      break;                                                                   \
    }                                                                          \
    case 6: {                                                                  \
      PARSER_PRINTF_INST("END");                                               \
      break;                                                                   \
    }                                                                          \
    case 7: {                                                                  \
      PARSER_PRINTF_INST("RET");                                               \
      break;                                                                   \
    }                                                                          \
    case 8: {                                                                  \
      PARSER_PRINTF_INST("DROP");                                              \
      break;                                                                   \
    }                                                                          \
    case 9: {                                                                  \
      PARSER_PRINTF_INST("DUP");                                               \
      break;                                                                   \
    }                                                                          \
    case 10: {                                                                 \
      PARSER_PRINTF_INST("SWAP");                                              \
      break;                                                                   \
    }                                                                          \
    case 11: {                                                                 \
      PARSER_PRINTF_INST("ELEM");                                              \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      _PARSER_FAIL_UNSUPPORTED_INST;                                           \
    }                                                                          \
    }                                                                          \
    break;                                                                     \
  }                                                                            \
  case 2:                                                                      \
  case 3:                                                                      \
  case 4: {                                                                    \
    PARSER_PRINTF_INST("%s\t", lds[h - 2]);                                    \
    switch (l) {                                                               \
    case 0: {                                                                  \
      _PARSER_NEXT_INT(i);                                                     \
      PARSER_PRINTF_INST("G(%d)", i);                                          \
      break;                                                                   \
    }                                                                          \
    case 1: {                                                                  \
      _PARSER_NEXT_INT(i);                                                     \
      PARSER_PRINTF_INST("L(%d)", i);                                          \
      break;                                                                   \
    }                                                                          \
    case 2: {                                                                  \
      _PARSER_NEXT_INT(i);                                                     \
      PARSER_PRINTF_INST("A(%d)", i);                                          \
      break;                                                                   \
    }                                                                          \
    case 3: {                                                                  \
      _PARSER_NEXT_INT(i);                                                     \
      PARSER_PRINTF_INST("C(%d)", i);                                          \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      _PARSER_FAIL_UNSUPPORTED_INST;                                           \
    }                                                                          \
    }                                                                          \
    break;                                                                     \
  }                                                                            \
  case 5:                                                                      \
    switch (l) {                                                               \
    case 0: {                                                                  \
      _PARSER_NEXT_LABEL(l);                                                   \
      PARSER_PRINTF_INST("CJMPz\t0x%.8x", l);                                  \
      break;                                                                   \
    }                                                                          \
    case 1: {                                                                  \
      _PARSER_NEXT_LABEL(l);                                                   \
      PARSER_PRINTF_INST("CJMPnz\t0x%.8x", l);                                 \
      break;                                                                   \
    }                                                                          \
    case 2: {                                                                  \
      _PARSER_NEXT_INT(nargs);                                                 \
      _PARSER_NEXT_INT(nlocals);                                               \
      PARSER_PRINTF_INST("BEGIN\t%d ", nargs);                                 \
      PARSER_PRINTF_INST("%d", nlocals);                                       \
      break;                                                                   \
    }                                                                          \
    case 3: {                                                                  \
      _PARSER_NEXT_INT(nargs);                                                 \
      _PARSER_NEXT_INT(nlocals);                                               \
      PARSER_PRINTF_INST("CBEGIN\t%d ", nargs);                                \
      PARSER_PRINTF_INST("%d", nlocals);                                       \
      break;                                                                   \
    }                                                                          \
    case 4: {                                                                  \
      _PARSER_NEXT_LABEL(entry);                                               \
      PARSER_PRINTF_INST("CLOSURE\t0x%.8x", entry);                            \
      {                                                                        \
        _PARSER_NEXT_INT(n);                                                   \
        for (int i = 0; i < n; i++) {                                          \
          _PARSER_NEXT_BYTE(designation);                                      \
          switch (designation) {                                               \
          case 0: {                                                            \
            _PARSER_NEXT_INT(i);                                               \
            PARSER_PRINTF_INST("G(%d)", i);                                    \
            break;                                                             \
          }                                                                    \
          case 1: {                                                            \
            _PARSER_NEXT_INT(i);                                               \
            PARSER_PRINTF_INST("L(%d)", i);                                    \
            break;                                                             \
          }                                                                    \
          case 2: {                                                            \
            _PARSER_NEXT_INT(i);                                               \
            PARSER_PRINTF_INST("A(%d)", i);                                    \
            break;                                                             \
          }                                                                    \
          case 3: {                                                            \
            _PARSER_NEXT_INT(i);                                               \
            PARSER_PRINTF_INST("C(%d)", i);                                    \
            break;                                                             \
          }                                                                    \
          default: {                                                           \
            _PARSER_FAIL_UNSUPPORTED_INST;                                     \
          }                                                                    \
          }                                                                    \
        }                                                                      \
      };                                                                       \
      break;                                                                   \
    }                                                                          \
    case 5: {                                                                  \
      _PARSER_NEXT_INT(nargs);                                                 \
      PARSER_PRINTF_INST("CALLC\t%d", nargs);                                  \
      break;                                                                   \
    }                                                                          \
    case 6: {                                                                  \
      _PARSER_NEXT_LABEL(l);                                                   \
      _PARSER_NEXT_INT(nargs);                                                 \
      PARSER_PRINTF_INST("CALL\t0x%.8x ", l);                                  \
      PARSER_PRINTF_INST("%d", nargs);                                         \
      break;                                                                   \
    }                                                                          \
    case 7: {                                                                  \
      _PARSER_NEXT_STRING(s);                                                  \
      _PARSER_NEXT_INT(nargs);                                                 \
      PARSER_PRINTF_INST("TAG\t%s ", s);                                       \
      PARSER_PRINTF_INST("%d", nargs);                                         \
      break;                                                                   \
    }                                                                          \
    case 8: {                                                                  \
      _PARSER_NEXT_INT(nelems);                                                \
      PARSER_PRINTF_INST("ARRAY\t%d", nelems);                                 \
      break;                                                                   \
    }                                                                          \
    case 9: {                                                                  \
      _PARSER_NEXT_INT(line);                                                  \
      _PARSER_NEXT_INT(col);                                                   \
      PARSER_PRINTF_INST("FAIL\t%d", line);                                    \
      PARSER_PRINTF_INST("%d", col);                                           \
      break;                                                                   \
    }                                                                          \
    case 10: {                                                                 \
      _PARSER_NEXT_INT(line);                                                  \
      PARSER_PRINTF_INST("LINE\t%d", line);                                    \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      _PARSER_FAIL_UNSUPPORTED_INST;                                           \
    }                                                                          \
    }                                                                          \
    break;                                                                     \
  case 6: {                                                                    \
    PARSER_PRINTF_INST("PATT\t%s", pats[l]);                                   \
    break;                                                                     \
  }                                                                            \
  case 7: {                                                                    \
    switch (l) {                                                               \
    case 0: {                                                                  \
      PARSER_PRINTF_INST("CALL\tLread");                                       \
      break;                                                                   \
    }                                                                          \
    case 1: {                                                                  \
      PARSER_PRINTF_INST("CALL\tLwrite");                                      \
      break;                                                                   \
    }                                                                          \
    case 2: {                                                                  \
      PARSER_PRINTF_INST("CALL\tLlength");                                     \
      break;                                                                   \
    }                                                                          \
    case 3: {                                                                  \
      PARSER_PRINTF_INST("CALL\tLstring");                                     \
      break;                                                                   \
    }                                                                          \
    case 4: {                                                                  \
      _PARSER_NEXT_INT(nargs);                                                 \
      PARSER_PRINTF_INST("CALL\tBarray\t%d", nargs);                           \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      _PARSER_FAIL_UNSUPPORTED_INST;                                           \
    }                                                                          \
    }                                                                          \
    break;                                                                     \
  }                                                                            \
  default: {                                                                   \
    _PARSER_FAIL_UNSUPPORTED_INST;                                             \
  }                                                                            \
  }

} // namespace lama::parser
