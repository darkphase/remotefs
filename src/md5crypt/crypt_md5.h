#ifndef CRYPT_MD5_H
#define CRYPT_MD5_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

char * md5_crypt(const char *pw, const char *salt);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CRYPT_MD5_H */
