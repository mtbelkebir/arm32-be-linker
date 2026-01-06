int IsHostBigEndian() {
  const long x = 1;
  return *(char*)&x == 0;
}

int main() {
  int x = 0;
  if (IsHostBigEndian()) {
    x = 1;
  }
  x++;
}