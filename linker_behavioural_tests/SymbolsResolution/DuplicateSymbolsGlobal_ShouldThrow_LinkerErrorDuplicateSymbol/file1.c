int IsHostBigEndian() {
  const long x = 1;
  return *(char*)&x == 0;
}