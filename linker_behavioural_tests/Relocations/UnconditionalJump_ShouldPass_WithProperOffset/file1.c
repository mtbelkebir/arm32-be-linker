void Target(void);

int main(void) {
  int x = 0;
  loop:
      x++;
  if (x < 5) {
    goto loop; // Génère une relocation R_ARM_JUMP24 interne
  }
  Target();     // Génère une relocation R_ARM_CALL externe
  return x;
}