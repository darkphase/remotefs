
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"
#include "client_ent.h"
#include "client_for_server.h"
#include "config.h"

int main()
{
	/*
	uid_t myuid = -1;

	DEBUG("myuid: %u, mypid: %u\n", myuid, getpid());

	if (rfsnss_is_server_running(myuid) != 0)
	{
		int inc_ret = rfsnss_addserver(myuid, "127.0.0.1");
		DEBUG("inc ret: %s\n", strerror(-inc_ret));
		if (inc_ret != 0)
		{
			return 1;
		}
	
		int dec_ret = rfsnss_dec(myuid);
		DEBUG("dec ret: %s\n", strerror(-dec_ret));
		if (dec_ret != 0)
		{
			return 1;
		}

		struct passwd *pwuid = rfsnss_getpwuid(10000);
		if (pwuid == NULL)
		{
			return 1;
		}
		DEBUG("received user name: %s\n", pwuid->pw_name);

		struct passwd *pwnam = rfsnss_getpwnam("alex@127.0.0.1");
		if (pwnam == NULL)
		{
			return 1;
		}
		DEBUG("received user uid: %d\n", pwnam->pw_uid);

		struct group *grgid = rfsnss_getgrgid(10000);
		if (grgid == NULL)
		{
			return 1;
		}
		DEBUG("received group name: %s\n", grgid->gr_name);

		struct group *grnam = rfsnss_getgrnam("alex@127.0.0.1");
		if (grnam == NULL)
		{
			return 1;
		}
		DEBUG("received group gid: %d\n", grnam->gr_gid);

		rfsnss_setpwent();
		struct passwd *pw_ent = NULL;
		do
		{
			pw_ent = rfsnss_getpwent();

			if (pw_ent != NULL)
			{
				DEBUG(">>> received username %s with uid %d\n", pw_ent->pw_name, pw_ent->pw_uid);
			}
		}
		while (pw_ent != NULL);
		rfsnss_endpwent();

		rfsnss_setgrent();
		struct group *gr_ent = NULL;
		do
		{
			gr_ent = rfsnss_getgrent();

			if (gr_ent != NULL)
			{
				DEBUG(">>> received groupname %s with gid %d\n", gr_ent->gr_name, gr_ent->gr_gid);
			}
		}
		while (gr_ent != NULL);
		rfsnss_endgrent();
	}
	*/
	
	setpwent();
	while (1)
	{
		struct passwd *pwd = getpwent();

		if (pwd == NULL)
		{
			break;
		}

		printf(">>> %s, %d, %d\n", pwd->pw_name, pwd->pw_uid, pwd->pw_gid);
	}
	/*
	endpwent();
	*/
	
	while (1)
	{
		struct group *grp = getgrent();

		if (grp == NULL)
		{
			break;
		}

		printf("<<< %s, %d\n", grp->gr_name, grp->gr_gid);
	}

	return 0;
}

