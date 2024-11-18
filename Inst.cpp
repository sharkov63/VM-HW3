#include "Inst.h"
#include <cassert>
#include <cstring>

using namespace lama;

int32_t InstRef::getParam(size_t index) const {
  assert(index < paramNum);
  int32_t result;
  memcpy(&result, data + 1, sizeof(int32_t));
  return result;
}
