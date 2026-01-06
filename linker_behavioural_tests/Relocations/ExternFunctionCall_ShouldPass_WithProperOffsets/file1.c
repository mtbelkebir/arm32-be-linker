extern int getSecret();

int main() {
  int isSecretValid = 0;

  if (getSecret() == 0xDEADBEEF) {
    isSecretValid = 1;
  }
}