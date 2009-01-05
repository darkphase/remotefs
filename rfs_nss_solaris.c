/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/* Solaris interface for name services */

NSS_STATUS _nss_rfs_default_destr(nss_backend_t *be, void *args);
NSS_STATUS _nss_rfs_not_found (nss_backend_t *be, void *args);
NSS_STATUS _nss_rfs_success(nss_backend_t *be, void *args);

NSS_STATUS _nss_rfs_sol_getpwnam_r(nss_backend_t *be, void *args)
{
    NSS_STATUS status;
    const char    *name    =  NSS_ARGS(args)->key.name;
    struct passwd *result  =  NSS_ARGS(args)->buf.result;
    char          *buffer  =  NSS_ARGS(args)->buf.buffer;
    size_t         buflen  =  NSS_ARGS(args)->buf.buflen;
    int           *errnop  = &NSS_ARGS(args)->erange;
    status = _nss_rfs_getpwnam_r(name, result, buffer, buflen, errnop);

    if (status == NSS_SUCCESS)
        NSS_ARGS(args)->returnval = NSS_ARGS(args)->buf.result;
    return status;
}

NSS_STATUS _nss_rfs_sol_getpwuid_r(nss_backend_t *be, void *args)
{
    NSS_STATUS status;
    uid_t          uid     =  NSS_ARGS(args)->key.uid;
    struct passwd *result  =  NSS_ARGS(args)->buf.result;
    char          *buffer  =  NSS_ARGS(args)->buf.buffer;
    size_t         buflen  =  NSS_ARGS(args)->buf.buflen;
    int           *errnop  = &NSS_ARGS(args)->erange;
    status = _nss_rfs_getpwuid_r(uid, result, buffer, buflen, errnop);

    if (status == NSS_SUCCESS)
        NSS_ARGS(args)->returnval = NSS_ARGS(args)->buf.result;
    return status;
}

NSS_STATUS _nss_rfs_sol_getgrnam_r(nss_backend_t *be, void *args)
{
    NSS_STATUS status;
    const char    *name    =  NSS_ARGS(args)->key.name;
    struct group  *result  =  NSS_ARGS(args)->buf.result;
    char          *buffer  =  NSS_ARGS(args)->buf.buffer;
    size_t         buflen  =  NSS_ARGS(args)->buf.buflen;
    int           *errnop  = &NSS_ARGS(args)->erange;
    status = _nss_rfs_getgrnam_r(name, result, buffer, buflen, errnop);

    if (status == NSS_SUCCESS)
        NSS_ARGS(args)->returnval = NSS_ARGS(args)->buf.result;
    return status;
}

NSS_STATUS _nss_rfs_sol_getgrgid_r(nss_backend_t *be, void *args)
{
    NSS_STATUS status;
    uid_t          uid     =  NSS_ARGS(args)->key.uid;
    struct group  *result  =  NSS_ARGS(args)->buf.result;
    char          *buffer  =  NSS_ARGS(args)->buf.buffer;
    size_t         buflen  =  NSS_ARGS(args)->buf.buflen;
    int           *errnop  = &NSS_ARGS(args)->erange;
    status = _nss_rfs_getgrgid_r(uid, result, buffer, buflen, errnop);

    if (status == NSS_SUCCESS)
        NSS_ARGS(args)->returnval = NSS_ARGS(args)->buf.result;
    return status;
}

static nss_backend_op_t passwd_ops[] =
{
    _nss_rfs_default_destr,       /* NSS_DBOP_DESTRUCTOR */
    _nss_rfs_success,             /* NSS_DBOP_ENDENT */
    _nss_rfs_success,             /* NSS_DBOP_SETENT */
    _nss_rfs_not_found,           /* NSS_DBOP_GETENT */
    _nss_rfs_sol_getpwnam_r,      /* NSS_DBOP_PASSWD_BYNAME */
    _nss_rfs_sol_getpwuid_r,      /* NSS_DBOP_PASSWD_BYUID */
};

nss_backend_t *_nss_rfs_passwd_constr (const char *db_name,
                                       const char *src_name,
                                       const char *cfg_args)
{
    nss_backend_t *be;
    be = (nss_backend_t *) malloc (sizeof (*be));
    if (!be)
        return NULL;
    be->ops = passwd_ops;
    be->n_ops = sizeof (passwd_ops) / sizeof (nss_backend_op_t);
    return be;
}

static nss_backend_op_t group_ops[] =
{
    _nss_rfs_default_destr,       /* NSS_DBOP_DESTRUCTOR */
    _nss_rfs_success,             /* NSS_DBOP_ENDENT */
    _nss_rfs_success,             /* NSS_DBOP_SETENT */
    _nss_rfs_not_found,           /* NSS_DBOP_GETENT */
    _nss_rfs_sol_getgrnam_r,      /* NSS_DBOP_GROUP_BYNAME */
    _nss_rfs_sol_getgrgid_r,      /* NSS_DBOP_GROUP_BYGID */
    _nss_rfs_not_found            /* NSS_DBOP_GROUP_BYMEMBER */
};

nss_backend_t *_nss_rfs_group_constr(const char *db_name,
                                     const char *src_name,
                                     const char *cfg_args)
{
    nss_backend_t *be;
    be = (nss_backend_t *) malloc (sizeof (*be));
    if (!be)
        return NULL;
    be->ops = group_ops;
    be->n_ops = sizeof (group_ops) / sizeof (nss_backend_op_t);
    return be;
}

NSS_STATUS _nss_rfs_default_destr(nss_backend_t *be, void *args)
{
    if (be)
    {
        free (be);
        be = NULL;
    }
    return NSS_SUCCESS;
}

NSS_STATUS _nss_rfs_not_found(nss_backend_t *be, void *args)
{
    return NSS_NOTFOUND;
}

NSS_STATUS _nss_rfs_success(nss_backend_t *be, void *args)
{
    return NSS_SUCCESS;
}
