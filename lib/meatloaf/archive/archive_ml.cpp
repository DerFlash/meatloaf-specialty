
#include "archive_ml.h"

#include <archive.h>
#include <archive_entry.h>

#include "meat_io.h"

// TODO: check how we can use archive_seek_callback, archive_passphrase_callback etc. to our benefit!

/* Returns pointer and size of next block of data from archive. */
// The read callback returns the number of bytes read, zero for end-of-file, or a negative failure code as above.
// It also returns a pointer to the block of data read.
ssize_t myRead(struct archive *a, void *__src_stream, const void **buff)
{
    MStream *src_str = (MStream *)__src_stream;

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
    MStream *src_str = (MStream *)__src_stream;

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
    MStream *src_str = (MStream *)__src_stream;
    if (src_str->isOpen())
    {
        bool rc = src_str->seek(request, SEEK_CUR);
        return (rc) ? request : ARCHIVE_WARN;
    }
    else
    {
        return ARCHIVE_FATAL;
    }
}

/********************************************************
 * Streams implementations
 ********************************************************/

ArchiveStream::ArchiveStream(std::shared_ptr<MStream> srcStr)
{
    srcStream = srcStr;
    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    srcBuffer = new uint8_t[buffSize];
}

ArchiveStream::~ArchiveStream()
{
    close();
    if (srcBuffer != nullptr)
        delete[] srcBuffer;
}

bool ArchiveStream::open()
{
    if (!is_open)
    {
        // callbacks set here:
        //                                    open, read  , skip,   close
        int r = archive_read_open2(a, srcStream.get(), NULL, myRead, myskip, myclose);
        if (r == ARCHIVE_OK)
            is_open = true;
    }
    return is_open;
};

void ArchiveStream::close()
{
    if (is_open)
    {
        archive_read_free(a);
        is_open = false;
    }
}

bool ArchiveStream::isOpen()
{
    return is_open;
};

uint32_t ArchiveStream::read(uint8_t *buf, uint32_t size)
{
    // ok so here we will basically need to refill buff with consecutive
    // calls to srcStream.read, I assume buf is filled by myread callback
    size_t r = archive_read_data(a, buf, size); // calls myread?
    _position += r;
    return r;
}

uint32_t ArchiveStream::write(const uint8_t *buf, uint32_t size)
{
    return -1;
}

// For files with a browsable random access directory structure
// d64, d74, d81, dnp, etc.
bool ArchiveStream::seekPath(std::string path)
{
    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        std::string entryName = archive_entry_pathname(entry);
        if (mstr::compare(path, entryName))
            return true;
    }
    return false;
};

// For files with no directory structure
// tap, crt, tar
std::string ArchiveStream::seekNextEntry()
{
    struct archive_entry *entry;
    if (archive_read_next_header(a, &entry) == ARCHIVE_OK)
        return archive_entry_pathname(entry);
    else
        return "";
};

uint32_t ArchiveStream::position()
{
    return _position;
}

// STREAM HAS NO IDEA HOW MUCH IS AVALABLE - IT'S ENDLESS!
uint32_t ArchiveStream::available()
{
    return 0; // whatever...
}

// STREAM HAS NO IDEA ABOUT SIZE - IT'S ENDLESS!
uint32_t ArchiveStream::size()
{
    return -1;
}

// WHAT DOES THIS FUNCTION DO???
size_t ArchiveStream::error()
{
    return -1;
}

bool ArchiveStream::seek(uint32_t pos)
{
    return srcStream->seek(pos);
}

/********************************************************
 * Files implementations
 ********************************************************/

MStream *ArchiveContainerFile::createIStream(std::shared_ptr<MStream> containerIstream)
{
    return new ArchiveStream(containerIstream);
}

// archive file is always a directory
bool ArchiveContainerFile::isDirectory()
{
    return true;
};

bool ArchiveContainerFile::rewindDirectory()
{
    if (a != nullptr)
        archive_read_free(a);

    return prepareDirListing();
}

MFile *ArchiveContainerFile::getNextFileInDir()
{
    struct archive_entry *entry;

    if (a == nullptr)
    {
        prepareDirListing();
    }

    if (a != nullptr)
    {
        if (archive_read_next_header(a, &entry) == ARCHIVE_OK)
        {
            auto newFile = MFSOwner::File(archive_entry_pathname(entry));
            // TODO - we can probably fill newFile with some info that is
            // probably available in archive_entry structure!
            newFile->size(archive_entry_size(entry)); // etc.

            return newFile;
        }
        else
        {
            archive_read_free(a);
            a = nullptr;
            return nullptr;
        }
    }
    else
    {
        return nullptr;
    }
}

bool ArchiveContainerFile::prepareDirListing()
{
    if (dirStream.get() != nullptr)
    {
        dirStream->close();
    }

    dirStream = std::shared_ptr<MStream>(this->meatStream());
    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    int r = archive_read_open2(a, dirStream.get(), NULL, myRead, myskip, myclose);
    if (r == ARCHIVE_OK)
    {
        return true;
    }
    else
    {
        archive_read_free(a);
        a = nullptr;
        return false;
    }
}
