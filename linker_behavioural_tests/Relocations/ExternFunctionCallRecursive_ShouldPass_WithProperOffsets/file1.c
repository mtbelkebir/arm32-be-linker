extern void Pong(int);


unsigned int Factorial(const unsigned int x) {
  if (x == 0) return 1;
  return x * Factorial(x - 1);
}

void Ping(int x) {
  if (x == 0) return;
  Pong(x - 1);
}