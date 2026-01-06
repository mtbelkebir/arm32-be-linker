extern void Ping(int);

void Pong(unsigned int x) {
  if (x == 0) return;
  Ping(x - 1);
}