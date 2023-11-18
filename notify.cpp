#include "pch.h"

#include "notify.h"
#include "util.h"

Notify::Notify( std::wstring& wpath ) :
	root_wpath ( wpath ),
	io_pending(false),
	dir_change_buffer(nullptr),
	overlap_notify(nullptr)
{
}

Notify::Notify(LPWSTR wpath, DWORD len) {
	root_wpath.assign(wpath, len);
	dir_change_buffer = nullptr;
	io_pending = false;
	overlap_notify = nullptr;
}

Notify::~Notify() {
	
	if (dir_change_buffer != nullptr) {
		HeapFree(GetProcessHeap(), 0, dir_change_buffer);
	}

	if (overlap_notify != nullptr) {
		HeapFree(GetProcessHeap(), 0, overlap_notify);
	}



	CloseHandle(root_handle);
}

int
Notify::InitHandle() {

	//this is completely by passing raii 
	//TODO: change this if switched to exception in the future
	dir_change_buffer = (char*) HeapAlloc(GetProcessHeap(), 0, DIR_CHANGE_BUFF_SIZE);
	overlap_notify = (OVERLAPPED_NOTIFY*)HeapAlloc(GetProcessHeap(), 0, sizeof(OVERLAPPED_NOTIFY));

	root_handle = CreateFileW(	root_wpath.c_str(), 
								FILE_LIST_DIRECTORY, 
								FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
								nullptr, 
								OPEN_EXISTING, 
								FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 
								nullptr );

	if (root_handle == INVALID_HANDLE_VALUE) {
		return -1;
	}
	return 0;
}

bool
Notify::HasEvent() const {
	return !event_queue.empty();
}

int
Notify::GetNextEvent(NotifyEvent *out) {
	*out = event_queue.front();
	event_queue.pop();
	return 1;
}

int 
Notify::PeekNextEvent(NotifyEvent* out) const {
	*out = event_queue.front();
	return 1;
}

int		
Notify::UpdateNextEventTypeToSkip() {
	if (event_queue.front().event == NotifyEvent::CREATE) {
		event_queue.front().event = NotifyEvent::CREATE_SKIP;
		return 1;
	}

	if (event_queue.front().event == NotifyEvent::RENAME_NEW) {
		event_queue.front().event = NotifyEvent::RENAME_SKIP;
		return 1;
	}

	return -1;
	
}

int Notify::RequestChanges() {

	
	DWORD bytes_returned; //not used in async

	overlap_notify->notifier = this;
	
	int ret = ReadDirectoryChangesW(	root_handle, 
										dir_change_buffer, 
										DIR_CHANGE_BUFF_SIZE, 
										true, 
										FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION, 
										&bytes_returned, 
										(LPOVERLAPPED) overlap_notify, 
										(LPOVERLAPPED_COMPLETION_ROUTINE) &ReadDirChangeCompleteRoutine);

	if (ret < 0) {
		//TODO: error
		return GetLastError();
	}


	return 1;
}

int
Notify::ProcessEventBuffer(unsigned int data_size) {

	NotifyEvent notify_event_buff;
	FILE_NOTIFY_INFORMATION *ptr = (FILE_NOTIFY_INFORMATION*) dir_change_buffer;
	int notify_amount = 0;
	
	for (;;) {

		FormNotifyEvent(ptr, &notify_event_buff);

		//printf("Dir: %S\nName: %S\n", notify_event_buff.dir_wname.c_str(), notify_event_buff.file_wname.c_str());
		event_queue.push(notify_event_buff);
		notify_amount++;

		if (ptr->NextEntryOffset == 0) {
			break;
		}

		ptr = (FILE_NOTIFY_INFORMATION*)( (char*) ptr + ptr->NextEntryOffset);
	}

	//printf("%d notices processed this routine\n", notify_amount);

	return 1;
}

int
Notify::FormNotifyEvent(FILE_NOTIFY_INFORMATION* f_notify, NotifyEvent *out) {

	ZeroMemory(out, sizeof(NotifyEvent));

	out->event = (NotifyEvent::EventType)f_notify->Action;

	//FileNameLength is always in bytes
	//To call WIDE CHAR function, FileNameLength has to be divided by 2 because each WIDE CHAR is 2 bytes big
	//filename field is not null terminated
	PathUtil::SplitDirFilenameCW(f_notify->FileName, ( f_notify->FileNameLength ) / 2 , out->dir_wname, out->file_wname);

	return 1;
}

void EventTypeToString(const NotifyEvent::EventType event, std::wstring *result) {
	switch (event) {
		case NotifyEvent::CREATE:
		case NotifyEvent::CREATE_SKIP:
			result->assign(L"CREATE");
			break;
		case NotifyEvent::REMOVE:
			result->assign(L"REMOVE");
			break;
		case NotifyEvent::MODIFY:
			result->assign(L"MODIFY");
			break;
		case NotifyEvent::RENAME_OLD:
			result->assign(L"RENAME_OLD");
			break;
		case NotifyEvent::RENAME_NEW:
		case NotifyEvent::RENAME_SKIP:
			result->assign(L"RENAME_NEW");
			break;
		default:
			result->assign(L"ERR");
	}
}

void WINAPI
ReadDirChangeCompleteRoutine(DWORD error, DWORD bytes_transfered, LPOVERLAPPED overlapped) {
	if (error != 0) {
		return;
	}

	if (bytes_transfered == 0) {
		return;
	}

	OVERLAPPED_NOTIFY *overlap_notify = (OVERLAPPED_NOTIFY*)overlapped;
	overlap_notify->notifier->ProcessEventBuffer(bytes_transfered);
}