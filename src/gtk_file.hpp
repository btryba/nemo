#ifndef __GTK_FILE_HPP__
#define __GTK_FILE_HPP__

#include <gio/gio.h>

namespace gtk
{
    struct File
    {
        GFile* file;
        public:
        File(const char* path);
        File(GUserDirectory userDirectory);
        //Set callAddReference to false if using a g command that automatically adds a ref for you.
        File(GFile* original, bool callAddReference = true);
        File();
        virtual ~File();

        bool operator==(File otherFile);
        bool operator!=(File otherFile);
        GFile* getPtr();
        //Checks the beginning of the path so if /foo is the prefix /foo/bar will return true
        bool has_prefix(File* prefix);
        void updatePath(const char* newPath);
        void updatePath(GFile* newPointer);
        void updatePath(File& newFile);
        char* get_uri();
        
        bool isNull();
    };
}

#endif //__GTK_FILE_HPP__