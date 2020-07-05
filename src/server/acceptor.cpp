#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include <event2/event.h>
#include <event2/listener.h>

#include "events.h"
#include "wrapper.h"
#include "acceptor.h"

using namespace std;

#define BUF_SIZE 1024

typedef void (*f_signal)(evutil_socket_t, short, void *);
static void
signal_cb(evutil_socket_t sig, short events, void *ctx)
{
	struct event_base *base = (struct event_base *)ctx;

	cout << "Caught an interrupt signal; exiting cleanly in two seconds. sig:" << sig << endl;

	event_base_loopexit(base, nullptr);
}

acceptor::acceptor(std::function<void()> &&f) : base(nullptr), listener(nullptr), signal_event(nullptr), holder(nullptr), breakCb(f)
{
}

int acceptor::init(int port)
{
	auto flags = EV_SIGNAL | EV_ET;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	base = obtain_event_base();

	listener = obtain_evconnlistener(base.get(), acceptor::connection_cb, (void *)this,
									 LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&sin, sizeof(sin));
	f_signal f = signal_cb;
	signal_event = obtain_event(base.get(), SIGINT, flags, f, (void *)base.get());
	if (!signal_event.get() || event_add(signal_event.get(), nullptr) < 0)
	{
		cout << "Could not create/add a signal event -!" << endl;
	}

	holder = std::make_shared<socketholder>();
	return 0;
}

void acceptor::connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx)
{
	if (ctx == nullptr)
		return;

	acceptor *accp = (acceptor *)ctx;
	accp->holder->onConnect(fd);
}

void acceptor::onListenBreak()
{
	if (breakCb)
		breakCb();
	holder->waitStop();
}

void acceptor::stop()
{
	event_base_loopexit(base.get(), nullptr);
}

void acceptor::wait()
{
	event_base_loop(this->base.get(), EVLOOP_NO_EXIT_ON_EMPTY);
	onListenBreak();
	cout << " exit the acceptor wait" << endl;
}
acceptor::~acceptor()
{
	cout << " Exit the acceptor" << endl;
}