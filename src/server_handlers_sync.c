
int handle_mknod(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_mknod(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_release(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_release(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_statfs(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_statfs(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_utime(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_utime(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_rename(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_rename(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_rmdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_rmdir(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_unlink(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_unlink(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_mkdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_mkdir(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_write(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_write(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_read(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_read(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_truncate(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_truncate(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_open(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_open(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_readdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_readdir(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_getattr(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_getattr(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_changepath(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_changepath(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_closeconnection(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_closeconnection(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}

int handle_auth(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _handle_auth(client_socket, client_addr, cmd);
		return keep_alive_unlock() == 0 ? ret : -1;
	}
	return -1;
}
