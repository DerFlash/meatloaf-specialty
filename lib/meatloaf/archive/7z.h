
// https://stackoverflow.com/questions/22543179/how-to-use-libarchive-properly
// https://libarchive.org/

#ifndef MEATLOAF_ARCHIVE_7Z
#define MEATLOAF_ARCHIVE_7Z

#include <sys/types.h>

#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "meat_io.h"

// TODO: check how we can use archive_seek_callback, archive_passphrase_callback etc. to our benefit!

/* Returns pointer and size of next block of data from archive. */
// The read callback returns the number of bytes read, zero for end-of-file, or a negative failure code as above. 
// It also returns a pointer to the block of data read.
ssize_t myRead(struct archive *a, void *__src_stream, const void **buff)
{
    MStream *src_str = (MStream*)__src_stream;

    // OK, so:
    // 1. we have to call srcStr.read(...)
    // 2. set *buff to the bufer read in 1.
    // 3. return read bytes count

    // *buff = srcBuffer;
    // return src_str->read(srcBuffer, buffSize);
    return -1;
}

int myclose(struct archive *a, void *__src_stream)
{
    MStream *src_str = (MStream*)__src_stream;

    // do we want to close srcStream here???
    return (ARCHIVE_OK);
}

/* 
It must return the number of bytes actually skipped, or a negative failure code if skipping cannot be done.
It can skip fewer bytes than requested but must never skip more.
Only positive/forward skips will ever be requested.
If skipping is not provided or fails, libarchive will call the read() function and simply ignore any data that it does not need.

* Skips at most request bytes from archive and returns the skipped amount.
* This may skip fewer bytes than requested; it may even skip zero bytes.
* If you do skip fewer bytes than requested, libarchive will invoke your
* read callback and discard data as necessary to make up the full skip.
*/
int64_t myskip(struct archive *a, void *__src_stream, int64_t request)
{
    MStream *src_str = (MStream*)__src_stream;
    if(src_str->isOpen()) {
        bool rc = src_str->seek(request, SEEK_CUR);
        return (rc) ? request : ARCHIVE_WARN;
    }
    else {
        return ARCHIVE_FATAL;
    }
}


/********************************************************
 * Streams implementations
 ********************************************************/

class ArchiveStream: public MStream {
    struct archive *a;
    bool is_open = false;
    size_t buffSize = 4096;
    uint8_t* srcBuffer = nullptr;
    uint32_t _position = 0;

public:

    ArchiveStream(std::shared_ptr<MStream> srcStr) {
        srcStream = srcStr;
        a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);
        srcBuffer = new uint8_t[buffSize];
    }

    ~ArchiveStream() {
        close();
        if(srcBuffer != nullptr)
            delete[] srcBuffer;
    }

    bool open() override {
        if(!is_open) {
            // callbacks set here:
            //                                    open, read  , skip,   close
            int r = archive_read_open2(a, srcStream, NULL, myRead, myskip, myclose);
            if (r == ARCHIVE_OK)
                is_open = true;
        }
        return is_open;
    };

    void close() override {
        if(is_open) {
            archive_read_free(a);
            is_open = false;
        }
    }

    bool isOpen() override {
        return is_open;
    };

    uint32_t read(uint8_t* buf, uint32_t size) override {
        // ok so here we will basically need to refill buff with consecutive
        // calls to srcStream.read, I assume buf is filled by myread callback
        size_t r = archive_read_data(a, buf, size); // calls myread?
        _position += r;
        return r;
    }

    uint32_t write(const uint8_t *buf, uint32_t size) override {
        return -1;
    }

    // For files with a browsable random access directory structure
    // d64, d74, d81, dnp, etc.
    bool seekPath(std::string path) override {
        struct archive_entry *entry;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            std::string entryName = archive_entry_pathname(entry);
            if(mstr::compare(path, entryName))
                return true;
        }
        return false;
    };

    // For files with no directory structure
    // tap, crt, tar
    std::string seekNextEntry() override {
        struct archive_entry *entry;
        if(archive_read_next_header(a, &entry) == ARCHIVE_OK)
            return archive_entry_pathname(entry);
        else
            return "";
    };

    uint32_t position() {
        return _position;
    }

    // no idea how to implement...
    uint32_t available() {
        return 0; // whatever...
    }

    virtual uint32_t size() override {
        return -1;
    }

    virtual size_t error() override {
        // huh???
        return -1;
    }

    virtual bool seek(uint32_t pos) override {
        return false;
    }

protected:
    std::shared_ptr<MStream> srcStream = nullptr; // a stream that is able to serve bytes of this archive

private:



};


/********************************************************
 * Files implementations
 ********************************************************/

class ArchiveContainerFile: public MFile
{
    struct archive *a = nullptr;

public:
    ArchiveContainerFile(std::string path) : MFile(path) {};

    MStream* createIStream(std::shared_ptr<MStream> containerIstream) {
        return new ArchiveStream(containerIstream);
    }

    bool isDirectory() {
        return true;
    };

    time_t getLastWrite() override ;
    time_t getCreationTime() override ;
    bool rewindDirectory() override {
        if(a != nullptr)
            archive_read_free(a);
            
        return prepareDirListing();
    }

    MFile* getNextFileInDir() override {
        struct archive_entry *entry;

        if(a == nullptr) {
            prepareDirListing();
        }

        if(a != nullptr) {
            if(archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                auto newFile = MFSOwner::File(archive_entry_pathname(entry));
                // TODO - we can probably fill newFile with some info that is
                // probably available in archive_entry structure!
                newFile->size(entry->ae_stat.aest_size); // etc.

                return newFile;
            }
            else {
                archive_read_free(a);
                a = nullptr;
                return nullptr;
            }
        }
        else {            
            return nullptr;
        }
    }

    bool mkDir() override ;
    bool exists() override ;
    uint32_t size() override ;
    bool remove() override ;
    bool rename(std::string dest);

private:
    bool prepareDirListing() {
        a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);
        int r = archive_read_open2(a, srcStr, NULL, myRead, myskip, myclose);
        if (r == ARCHIVE_OK) {
            return true;
        }
        else {
            archive_read_free(a);
            a = nullptr;
            return false;
        }
    }
};



/********************************************************
 * FS implementations
 ********************************************************/

class ArchiveContainerFileSystem: public MFileSystem
{
    MFile* getFile(std::string path) {
        return new ArchiveContainerFile(path);
    };


public:
    ArchiveContainerFileSystem(): MFileSystem("arch"){}

    bool handles(std::string fileName) {
        
        return byExtension(
            {
                ".7z",
                ".arc",
                ".ark",
                ".bz2",
                ".gz",
                ".lha",
                ".lzh",
                ".lzx",
                ".rar",
                ".tar",
                ".tgz",
                ".xar",
                ".zip"
            }, 
            fileName
        );
    }

private:
};



#endif // MEATLOAF_ARCHIVE_7Z