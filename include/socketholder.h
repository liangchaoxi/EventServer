#ifndef __SOCKET_HOLDER_H__
#define __SOCKET_HOLDER_H__

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <event2/event.h>
#include <event2/listener.h>

#include "events.h"
#include "wrapper.h"
#include "channel.h"
#include "threadpools.h"

#define READ_LOOP_MAX (8)
#define WRITE_LOOP_MAX (2)
#define EVENT_BASE_WATCH_MAX (128 * 1024)

using namespace std;
class socketholder : public std::enable_shared_from_this<socketholder>
{
private:
	std::array<raii_event_base, READ_LOOP_MAX> rwatchers;
	std::array<std::thread, READ_LOOP_MAX> watcher_thread;
	std::mutex syncMutex;
	std::condition_variable condition;
	std::atomic_bool isStop;
	std::array<std::map<evutil_socket_t, std::shared_ptr<channel>>, READ_LOOP_MAX> chns;
	threadpools pools;
	friend channel;

public:
	socketholder();
	virtual ~socketholder();
	void onConnect(evutil_socket_t fd);
	void onDisconnect(evutil_socket_t fd);
	size_t send(std::shared_ptr<channel> c, char *d, size_t l);
	std::shared_ptr<channel> getChannel(evutil_socket_t fd);
	void closeIdleChannel();
	void waitStop();

	std::shared_ptr<socketholder> share()
	{
		return shared_from_this();
	}
};
#endif /* __SOCKET_HOLDER_H__ */