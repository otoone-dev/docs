/*
  固定小数点の演算を扱います
*/

#if 0
#define UNIT_BITS (14)
#define UNIT_SIZE (1 << 14)
#deinfe MAX_INTVALUE (32767 / UNIT_SIZE)

class FixedPointNumber {
private:
  int value;
public:
  FixedPointNumber(int _value) {
    value = _value;
  }
  FixedPointNumber(float _value) {
    value = static_cast<int>(_value * UNIT_SIZE);
  }
  FixedPointNumber(double _value) {
    value = static_cast<int>(_value * UNIT_SIZE);
  }

  int operator + (FixedPointNumber fi) {
    return this.value + fi.value;
  } 
}
#endif
