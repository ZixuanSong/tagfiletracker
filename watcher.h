#pragma once
#include <string>
#include <Windows.h>

#include "notify.h"

class Watcher {
public:
	explicit Watcher(std::wstring& wpath);
	//C-string constructor
	explicit Watcher(LPWSTR, DWORD);
	~Watcher();

	int	WatcherInit();
	int	Watch();
private:
	std::wstring*		root_dir;
	Notify				notifier;
};

