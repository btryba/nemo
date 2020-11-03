#include "gtk_file.hpp"

namespace gtk
{
    File::File() : file{nullptr}
    {}

    File::File(const char* path)
    {
        file = g_file_new_for_path(path);
    }

    File::File(GUserDirectory userDirectory)
    {
        const gchar *special_dir  = g_get_user_special_dir (userDirectory);

        if (special_dir)
            file = g_file_new_for_path (special_dir);
        else
            file = nullptr;
    }

    bool File::operator==(File otherFile)
    {
        return g_file_equal(file, otherFile.getPtr());
    }

    GFile* File::getPtr()
    {
        return file;
    }

    File::~File()
    {
        g_object_unref(&file);
    }

    File::File(GFile* original, bool callAddReference)
    {
        if(callAddReference)
            file = (GFile*)g_object_ref(original);
        else
            file = original;
    }

    bool File::has_prefix(File* prefix)
    {
        return g_file_has_prefix(file, prefix->getPtr());
    }

    void File::updatePath(const char* newPath)
    {
        if(file != nullptr)
            g_object_unref(file);
        file = g_file_new_for_path(newPath);
    }

    void File::updatePath(GFile* newPointer)
    {
        if(file != nullptr)
            g_object_unref(file);
        file = newPointer;
    }

    void File::updatePath(File& newFile)
    {
        if(file != nullptr)
            g_object_unref(file);
        file = (GFile*)g_object_ref(newFile.getPtr());
    }

    bool File::isNull()
    {
        return file == nullptr;
    }

    char* File::get_uri()
    {
        return g_file_get_uri(file);
    }
}