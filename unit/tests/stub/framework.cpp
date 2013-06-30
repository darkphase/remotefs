
#include "framework.hpp"

std::auto_ptr<Stub::Framework> Stub::Framework::m_instance;

Stub::Framework::Framework()
{
	m_client.connect();
	m_server.accept();
}

Stub::Framework* 
Stub::Framework::get_instance()
{
	if (m_instance.get() == NULL)
	{
	    m_instance.reset(new Framework());
	}
	
	return m_instance.get();
}

void Stub::Framework::reset()
{
    m_instance.release();
}

Stub::Framework::ret_t 
Stub::Framework::communicate(handler_t handler, caller_t caller)
{
	ret_t ret(0, 0);
	
	pid_t client_pid = fork();
	if (client_pid != 0)
	{ // server
		command cmd = { 0 };
		m_server.recv_command(cmd);

		ret.handler = handler(m_server, cmd);

		int status = -1;
		waitpid(client_pid, &status, 0);

		if (WIFSIGNALED(status))
		{
			throw EXCEPTION(ECANCELED);
		}

		ret.caller = WEXITSTATUS(status);

		return ret;
	}
	else
	{ // client
		int ret = caller(m_client);
		exit(-ret);
	}
}
