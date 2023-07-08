#ifndef MEATLOAF_FILE
#define MEATLOAF_FILE

#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "../../include/debug.h"

//#include "wrappers/iec_buffer.h"

#include "meat_stream.h"
#include "peoples_url_parser.h"
#include "string_utils.h"
#include "U8Char.h"

#define _MEAT_NO_DATA_AVAIL -69

/********************************************************
 * Universal file
 ********************************************************/

class MFile : public PeoplesUrlParser {
public:
    MFile() {}; // only for local FS!!!
    MFile(nullptr_t null) : m_isNull(true) {};
    MFile(std::string path);
    MFile(std::string path, std::string name);
    MFile(MFile* path, std::string name);

    virtual ~MFile() {
        if(streamFile != nullptr) {
        //     Debug_printv("WARNING: streamFile null in '%s' destructor. This MFile was obviously not initialized properly!", url.c_str());
        // }
        // else {
            //Debug_printv("Deleting: [%s]", this->url.c_str());
            delete streamFile;
        }
    };

    MFile* parent(std::string = "");
    MFile* localParent(std::string);
    MFile* root(std::string);
    MFile* localRoot(std::string);

    std::string media_header;
    std::string media_id;
    std::string media_archive;
    std::string media_image;
    uint16_t media_blocks_free = 0;
    uint16_t media_block_size = 256;

    bool operator!=(nullptr_t ptr);

    // bool copyTo(MFile* dst);

    virtual std::string petsciiName() {
        std::string pname = name;
        mstr::toPETSCII(pname);
        return pname;
    }

    // has to return OPENED stream
    /**
     * The main matrioszka-stream magic happens here.
     *
     * This function needs to be overriden ONLY for "root" filesystems, meaning
     * such file systems that can't be contained in another filesystem, like
     * /, http:, ftp: and so on. For all other cases KEEP THE DEFAULT IMPLEMENTATION
     * which should be smart enough for most cases.
     * 
     * I repeat: DO NOT OVERRIDE THIS FUNCTION unless you're creating a FS that
     * can't be contained in another. Use createIStream() instead.
     *
     * @see createIStream()
     * @return already open stream for reading this particular file, i.e. if this
     * is a file inside some image or archive, the next byte read will be the first
     * byte of selected file inside the image, NOT the first byte of the image
     *              
     */
    virtual MStream* meatStream();

    virtual MFile* cd(std::string newDir);
    virtual bool isDirectory() = 0;
    virtual bool rewindDirectory() = 0 ;
    virtual MFile* getNextFileInDir() = 0 ;
    virtual bool mkDir() = 0 ;    

    virtual bool exists() = 0;
    virtual bool remove() = 0;
    virtual bool rename(std::string dest) = 0;    
    virtual time_t getLastWrite() = 0 ;
    virtual time_t getCreationTime() = 0 ;
    virtual size_t size() = 0;
    virtual uint64_t getAvailableSpace();

    virtual bool isText() {
        return mstr::isText(extension);
    }

    MFile* streamFile = nullptr;
    std::string pathInStream;

protected:
    /**
     * Return a specialized stream of data for this particular path
     * 
     * This is the function you need to override to provide a specific byte
     * stream for this particular file type. Note this method is called ONLY
     * by the default implementation of meatStream method, so if you have
     * overriden that one, you probably don't need to override this!
     * 
     * Let's say you want to implement a browsable D64 stream. The source D64
     * file might lie inside any other file sytem (HTTP, FTP, ZIP, local...)
     * and its bytes are provided by stream passed in src param.
     * So this is the place where you will return a smart stream that can
     * be used for seeking or skipping inside the D64 image.
     * 
     * @see meatStream()
     * @param src raw bytes of this container
     * @return specialized stream that handles this particular file/container
    */
    virtual MStream* createIStream(std::shared_ptr<MStream> src) = 0;
    bool m_isNull;

friend class MFSOwner;
};

/********************************************************
 * Filesystem instance
 * it knows how to create a MFile instance!
 ********************************************************/

class MFileSystem {
public:
    MFileSystem(const char* symbol);
    virtual ~MFileSystem() = 0;
    virtual bool mount() { return true; };
    virtual bool umount() { return true; };
    virtual bool handles(std::string path) = 0;
    virtual MFile* getFile(std::string path) = 0;
    bool isMounted() {
        return m_isMounted;
    }

    static bool byExtension(const char* ext, std::string fileName) {
        return mstr::endsWith(fileName, ext, false);
    }

protected:
    const char* symbol;
    bool m_isMounted;

    friend class MFSOwner;
};


/********************************************************
 * MFile factory
 ********************************************************/

class MFSOwner {
public:
    static std::vector<MFileSystem*> availableFS;

    static MFile* File(std::string name);
    static MFile* File(std::shared_ptr<MFile> file);
    static MFile* File(MFile* file);

    static MFileSystem* scanPathLeft(std::vector<std::string> paths, std::vector<std::string>::iterator &pathIterator);

    static std::string existsLocal( std::string path );
    static MFileSystem* testScan(std::vector<std::string>::iterator &begin, std::vector<std::string>::iterator &end, std::vector<std::string>::iterator &pathIterator);


    static bool mount(std::string name);
    static bool umount(std::string name);
};

/********************************************************
 * Meat namespace, standard C++ buffers and streams
 ********************************************************/

namespace Meat {
    struct _Unique_mf {
        typedef std::unique_ptr<MFile> _Single_file;
    };

    // Creates a unique_ptr<MFile> for a given url

    /**
    *  @brief  Creates a unique_ptr<MFile> instance froma given url
    *  @param  url  The url to the file.
    *  @return  @c unique_ptr<MFile>
    */
    template<class MFile>
        typename _Unique_mf::_Single_file
        New(std::string url) {
            return std::unique_ptr<MFile>(MFSOwner::File(url));
        }

    /**
    *  @brief  Creates a unique_ptr<MFile> instance froma given url
    *  @param  url  The url to the file.
    *  @return  @c unique_ptr<MFile>
    */
    template<class MFile>
        typename _Unique_mf::_Single_file
        New(const char* url) {
            return std::unique_ptr<MFile>(MFSOwner::File(std::string(url)));
        }

    /**
    *  @brief  Creates a unique_ptr<MFile> instance froma given MFile
    *  @param  file  The url to the file.
    *  @return  @c unique_ptr<MFile>
    */
    template<class MFile>
        typename _Unique_mf::_Single_file
        New(MFile* mFile) {
            return std::unique_ptr<MFile>(MFSOwner::File(mFile->url));
        }

    /**
    *  @brief  Wraps MFile* into unique_ptr<MFile> so it closes itself as required
    *  @param  file  The url to the file.
    *  @return  @c unique_ptr<MFile>
    */
    template<class MFile>
        typename _Unique_mf::_Single_file
        Wrap(MFile* file) {
            return std::unique_ptr<MFile>(file);
        }

}

#endif // MEATLOAF_FILE
