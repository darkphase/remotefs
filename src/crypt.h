#ifndef CRYPT_H
#define CRYPT_H

/* password encryption routine */

/** get hash of password */
char *passwd_hash(const char *password, const char *salt);

#endif // CRYPT_H
