#pragma once




typedef enum FileAction
{
    FileAction_Add = 0,
    FileAction_Delete,
    FileAction_Modified,

    NumFileActions
} FileAction;


struct _FileWatchID;
typedef struct _FileWatchID* FileWatchID;


typedef void(*FileWatchCB)(const char* dir, const char* filename, FileAction action);



void fileWatcherStartup(void);
void fileWatcherShutdown(void);
FileWatchID fileWatcherAddDir(const char* path, FileWatchCB cb);
void fileWatcherRemoveDir(const char* path);
void fileWatcherRemoveWatch(FileWatchID id);
void fileWatcherUpdate(double deltaTime);









































































