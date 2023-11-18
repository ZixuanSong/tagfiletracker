#pragma once

/*
	Library that utilizes Winapi ReadDirectoryChangesW
*/

#include <string>
#include <queue>
#include <unordered_map>

#include <Windows.h>

#define DWORD_ALIGN __declspec(align(sizeof DWORD))

//how big should the buffer be? hardest question in programming
//8kb for now
#define DIR_CHANGE_BUFF_SIZE (1024 * 8)

class Notify;

struct OVERLAPPED_NOTIFY {
	OVERLAPPED	overlapped;
	Notify*		notifier;
};



struct NotifyPathEntry {
	std::string entry_wpath;
};

struct NotifyEvent {

	enum EventType {

		CREATE = 1,
		REMOVE,
		MODIFY,
		RENAME_OLD,
		RENAME_NEW,
		CREATE_SKIP,
		RENAME_SKIP
	};

	EventType			event;
	//NotifyPathEntry*	entry;
	std::wstring		dir_wname;
	std::wstring		file_wname;
};

class Notify {
public:
	explicit Notify( std::wstring& );

	//C-string constructor
	explicit Notify(LPWSTR, DWORD);
	~Notify();

	int		InitHandle();
	bool	HasEvent() const;
	int		RequestChanges();
	int		GetNextEvent(NotifyEvent*);
	int		PeekNextEvent(NotifyEvent*) const;
	int		UpdateNextEventTypeToSkip();
	int		ProcessEventBuffer(unsigned int);
	int		FormNotifyEvent(FILE_NOTIFY_INFORMATION*, NotifyEvent*);

private:
	std::wstring					root_wpath;
	HANDLE							root_handle;
	std::vector<std::wstring>		wexclusion_wpaths;
	std::queue<NotifyEvent>			event_queue;

	bool							io_pending;		//useful for restarting daemon need to clear outstanding io result before restart. but not needed if daemon can't be restarted.
	char							*dir_change_buffer;
	OVERLAPPED_NOTIFY				*overlap_notify;
};

void EventTypeToString(const NotifyEvent::EventType, std::wstring*);
void WINAPI ReadDirChangeCompleteRoutine(DWORD, DWORD, LPOVERLAPPED);