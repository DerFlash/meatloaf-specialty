#ifndef MEATFILESYSTEM_WRAPPERS_IEC_BUFFER
#define MEATFILESYSTEM_WRAPPERS_IEC_BUFFER

#include "../../../include/debug.h"

#include "iec.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "U8Char.h"

#define IEC_BUFFER_SIZE 100 // 1024

/********************************************************
 * oiecstream
 * 
 * Standard C++ stream for writing to IEC
 ********************************************************/

class oiecstream : private std::filebuf, public std::ostream {
    char* data;
    iecBus* m_iec;
    bool m_isOpen = false;

    size_t easyWrite();

public:
    oiecstream(const oiecstream &copied) : std::ios(0), std::filebuf(),  std::ostream( this ) {
        Debug_printv("oiecstream COPY constructor");
    }

    oiecstream() : std::ostream( this ) {
        Debug_printv("oiecstream constructor");

        data = new char[IEC_BUFFER_SIZE+1];
        setp(data, data+IEC_BUFFER_SIZE);
    };

    ~oiecstream() {
        Debug_printv("oiecstream destructor");

        close();
        
        if(data != nullptr)
            delete[] data;
    }


    virtual void open(iecBus* iec) {
        m_iec = iec;
        setp(data, data+IEC_BUFFER_SIZE);
        if(iec != nullptr)
        {
            m_isOpen = true;
            clear();
        }
    }

    virtual void close() {
        sync();

        if(pptr()-pbase() == 1) {
            char last = data[0];
            Debug_printv("closing, sending EOI with [%.2X] %c", last, last);
            //m_iec->sendEOI(0);
            setp(data, data+IEC_BUFFER_SIZE);
        }

        m_isOpen = false;
    }

    bool is_open() const {
        return m_isOpen;
    }    

    int overflow(int ch  = std::filebuf::traits_type::eof()) override;

    int sync() override;


    void putUtf8(U8Char* codePoint);
};

#endif /* MEATFILESYSTEM_WRAPPERS_IEC_BUFFER */
