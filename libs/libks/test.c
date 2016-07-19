#define MESSAGE (const unsigned char *) "test"
#define MESSAGE_LEN 4
#include <sodium.h>

int main() {

unsigned char pk[crypto_sign_PUBLICKEYBYTES];
unsigned char sk[crypto_sign_SECRETKEYBYTES];
crypto_sign_keypair(pk, sk);

unsigned char sig[crypto_sign_BYTES];

crypto_sign_detached(sig, NULL, MESSAGE, MESSAGE_LEN, sk);

if (crypto_sign_verify_detached(sig, MESSAGE, MESSAGE_LEN, pk) != 0) {
	printf("Failed!\n");
 } else {
	printf("Success\n");
 }

return 0;
}
