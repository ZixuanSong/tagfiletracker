#include "pch.h"
#include "watcher.h"



Watcher::Watcher(std::wstring& wpath) :
	notifier(wpath)
{
}

Watcher::Watcher(LPWSTR wpath, DWORD len) :
	notifier(wpath, len)
{
}

Watcher::~Watcher() {

}

int
Watcher::WatcherInit() {
	int ret = notifier.InitHandle();
	if (ret < 0) {
		return -1;
	}
	return 1;
}

int
Watcher::Watch() {

	NotifyEvent event;
	std::string event_type_str;
	int equal_sign_count = 0;
	int ret = 0;

	for (;;) {
		//ret = notifier.NextEvent(&event);
		if (ret < 0) {
			return -1;
		}

		//EventTypeToString(event, &event_type_str);

		//do things.... pretty print for now
		for (int i = 0; i < equal_sign_count; i++) {
			printf("=");
		}
		equal_sign_count = (++equal_sign_count) % 10;

		printf("\nEvent Type: %s\nDir: %S\nName: %S\n", event_type_str.c_str(), event.dir_wname.c_str(), event.file_wname.c_str());
	}
}
