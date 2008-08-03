#ifndef CRYPT_H
#define CRYPT_H

/** password encryption routine */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** get hash of password */
char *passwd_hash(const char *password, const char *salt);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CRYPT_H */
