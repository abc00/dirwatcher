#include <FileWatcher/FileWatcher.h>
#include <map>
#include <vector>
#include <assert.h>
#include <sys/stat.h>

#ifdef _WIN32
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif



extern "C"
{

#include "filewatcher.h"

}



enum
{
    PATH_BUF_MAX = 260,
};




typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

typedef signed char schar;
typedef signed short sshort;
typedef signed int sint;
typedef signed long slong;
typedef signed long long slonglong;

typedef float float32_t;
typedef double float64_t;



#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__ia64__)
# define PTR64
#endif

#ifdef PTR64
typedef float64_t floatptr_t;
#else
typedef float32_t floatptr_t;
#endif






static bool dirExist(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}







struct FileOP
{
    char dir[PATH_BUF_MAX];
    char filename[PATH_BUF_MAX];
    FileAction action;
    FileWatchCB cb;

    bool operator==(const FileOP& _b) const
    {
        const FileOP* a = this;
        const FileOP* b = &_b;
        return
            (0 == strncmp(a->dir, b->dir, PATH_BUF_MAX)) &&
            (0 == strncmp(a->filename, b->filename, PATH_BUF_MAX)) &&
            (a->action == b->action) &&
            (a->cb == b->cb);
    }
};





class FileWatchListener : public FW::FileWatchListener
{
    std::map<FW::WatchID, FileWatchCB> m_cbMap;

public:
    FileWatchListener() {}

    void addCB(FW::WatchID watchid, FileWatchCB cb)
    {
        m_cbMap.insert(std::pair<FW::WatchID, FileWatchCB>(watchid, cb));
    }
    void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action);
};


struct FileWatcher
{
    double maxTime;
    double time;
    FW::FileWatcher* hander;
    FileWatchListener* listener;
    std::vector<FileOP> fileOPs;

    FileWatcher() :
        maxTime(1.0),
        time(0.0),
        hander(new FW::FileWatcher()),
        listener(new FileWatchListener())
    {
    }
    ~FileWatcher()
    {
        delete listener;
        delete hander;
    }
};

static FileWatcher* fileWatcher = NULL;



void FileWatchListener::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action _action)
{
    FileAction action;
    switch (_action)
    {
    case FW::Actions::Add:
    {
        action = FileAction_Add;
        break;
    }
    case FW::Actions::Delete:
    {
        action = FileAction_Delete;
        break;
    }
    case FW::Actions::Modified:
    {
        action = FileAction_Modified;
        break;
    }
    default:
        assert(false);
        break;
    }
    //switch (action)
    //{
    //case FileAction_Add:
    //{
    //    debugLog("File (%s/%s) Added!", dir.c_str(), filename.c_str());
    //    break;
    //}
    //case FileAction_Delete:
    //{
    //    debugLog("File (%s/%s) Deleted!", dir.c_str(), filename.c_str());
    //    break;
    //}
    //case FileAction_Modified:
    //{
    //    debugLog("File (%s/%s) Modified!", dir.c_str(), filename.c_str());
    //    break;
    //}
    //default:
    //    assert(false);
    //    break;
    //}
    for (std::map<FW::WatchID, FileWatchCB>::const_iterator it = m_cbMap.begin(); it != m_cbMap.end(); ++it)
    {
        FileOP op = { 0 };
        strncpy(op.dir, dir.c_str(), dir.size());
        strncpy(op.filename, filename.c_str(), filename.size());
        op.cb = it->second;
        op.action = action;

        assert(fileWatcher);

        if (std::find(fileWatcher->fileOPs.begin(), fileWatcher->fileOPs.end(), op) == fileWatcher->fileOPs.end())
        {
            fileWatcher->fileOPs.push_back(op);
        }
    }
}





extern "C" void fileWatcherStartup(void)
{
    assert(!fileWatcher);
    fileWatcher = new FileWatcher();
}


extern "C" void fileWatcherShutdown(void)
{
    assert(fileWatcher);
    delete fileWatcher;
    fileWatcher = NULL;
}





extern "C" FileWatchID fileWatcherAddDir(const char* path, FileWatchCB cb)
{
    assert(fileWatcher);

    if (!dirExist(path))
    {
        printf("Failed watch dir \"%s\": doesn't exist\n", path);
        return NULL;
    }
    FW::WatchID watchid = fileWatcher->hander->addWatch(path, fileWatcher->listener);
    fileWatcher->listener->addCB(watchid, cb);
    return (FileWatchID)(uintptr_t)watchid;
}




extern "C" void fileWatcherRemoveDir(const char* path)
{
    assert(fileWatcher);
    fileWatcher->hander->removeWatch(path);
}




extern "C" void fileWatcherRemoveWatch(FileWatchID id)
{
    assert(fileWatcher);
    fileWatcher->hander->removeWatch((FW::WatchID)(uintptr_t)id);
}





extern "C" void fileWatcherUpdate(double now)
{
    assert(fileWatcher);

    fileWatcher->hander->update();

    if ((now - fileWatcher->time) > fileWatcher->maxTime)
    {
        fileWatcher->time = now;
        for (uint i = 0; i < fileWatcher->fileOPs.size(); ++i)
        {
            FileOP* op = &fileWatcher->fileOPs[i];
            op->cb(op->dir, op->filename, op->action);
        }
        fileWatcher->fileOPs.clear();
    }
}







































































































