

#include <cstdint>
#include <cstdio>
class GP {
public:
  explicit GP(uint32_t v) : v(v) {};
  GP operator +(const GP b) const { return GP(v ^ b.v); }
  GP operator -(const GP b) const { return GP(*this + b); }
  //GP operator *(const GP b) const { return GP((v * b.v) % 255u); }
  //GP operator *=(const GP b) { return *this = *this * b; }
  GP log() { return GP(1); }
  bool operator ==(const GP other) const { return v == other.v; }
  uint32_t value() const { return v; }

private:
  uint32_t v;
};

// Primitive polinomial x8 + x4 + x3 + x2 + 1 (285)
int main(int argc, char** argv) {
  uint64_t v = 1;
  printf("0, ");
  for (int x = 1; x < 256; x++) {
    printf("%ld, ", v % 285);
    v = v * 2;
    if (v > 255) v ^= 285;
  }
  printf("\n");
}
