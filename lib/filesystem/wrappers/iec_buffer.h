#ifndef MEATLOAF_WRAPPER_IEC_BUFFER
#define MEATLOAF_WRAPPER_IEC_BUFFER

#include "../../../include/debug.h"

#include "iec.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "U8Char.h"

#define IEC_BUFFER_SIZE 25 // 1024
/********************************************************
 * oiecbuf
 * 
 * A buffer for writing IEC data, handles sending EOI
 ********************************************************/

// class oiecbuf : public std::filebuf {
//     char* data;
//     IEC* m_iec;
//     bool m_isOpen = false;

//     size_t easyWrite(bool lastOne);

// public:
//     oiecbuf(const oiecbuf &b) : basic_filebuf( &b) {}
//     oiecbuf() {
//         Debug_printv("oiecbuffer constructor");

//         data = new char[IEC_BUFFER_SIZE];
//         setp(data, data+IEC_BUFFER_SIZE);
//     }

//     ~oiecbuf() {
//         Debug_printv("oiecbuffer desttructor");
//         if(data != nullptr)
//             delete[] data;

//         close();
//     }

//     bool is_open() const {
//         return m_isOpen;
//     }

//     virtual void open(IEC* iec) {
//         m_iec = iec;
//         if(iec != nullptr)
//             m_isOpen = true;
//     }

//     virtual void close() {
//         sync();
//         m_isOpen = false;
//     }

//     int overflow(int ch  = traits_type::eof()) override;

//     int sync() override;
// };




/********************************************************
 * oiecstream
 * 
 * Standard C++ stream for writing to IEC
 ********************************************************/

class oiecstream : private std::filebuf, public std::ostream {
    char* data;
    iecBus* m_iec;
    bool m_isOpen = false;

    size_t easyWrite(bool lastOne);

public:
    oiecstream(const oiecstream &copied) : std::ios(0), std::filebuf(),  std::ostream( this ) {
        Debug_printv("oiecstream COPY constructor");
    }

    oiecstream() : std::ostream( this ) {
        Debug_printv("oiecstream constructor");

        data = new char[IEC_BUFFER_SIZE];
        setp(data, data+IEC_BUFFER_SIZE);
    };

    ~oiecstream() {
        Debug_printv("oiecstream destructor");

        if(data != nullptr)
            delete[] data;

        close();
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
        m_isOpen = false;
    }

    bool is_open() const {
        return m_isOpen;
    }    

    int overflow(int ch  = std::filebuf::traits_type::eof()) override;

    int sync() override;


    void putUtf8(U8Char* codePoint);
};

#endif /* MEATLOAF_WRAPPER_IEC_BUFFER */
