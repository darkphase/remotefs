
#if ! defined _RFS_NSS_GERNAME_H_
#define _RFS_NSS_GETNAME_H_

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int get_all_names(char *ip_or_name);
int translate_ip(char *ip_or_name, char *host, size_t hlen);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif
