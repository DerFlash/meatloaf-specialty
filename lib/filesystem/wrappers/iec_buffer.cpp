#include "iec_buffer.h"

/********************************************************
 * oiecbuf
 * 
 * A buffer for writing IEC data, handles sending EOI
 ********************************************************/

size_t oiecstream::easyWrite(bool lastOne) {
    size_t written = 0;

    // we're always writing without the last character in buffer just to be able to send this special delay
    // if this is last character in the file
    auto count = pptr()-pbase();
    Debug_printv("IEC easyWrite will try to send %d bytes over IEC (but buffer contains %d)", count-1, count);

    //  pptr =  Returns the pointer to the current character (put pointer) in the put area.
    //  pbase = Returns the pointer to the beginning ("base") of the put area.
    //  epptr = Returns the pointer one past the end of the put area.
    for(auto b = pbase(); b < pptr()-1; b++) {
        //Serial.printf("%c",*b);
        bool sendSuccess = m_iec->send(*b);
        if(sendSuccess && !(IEC.protocol->flags bitand ATN_PULLED) ) written++;
        else if(!sendSuccess) {
            /*
            istream->good(); // rdstate == 0
            istream->bad(); // rdstate() & badbit) != 0
            istream->eof(); // rdstate() & eofbit) != 0
            istream->fail(); // >rdstate() & (badbit | failbit)) != 0
            istream->rdstate();
            istream->setstate();
            _S_goodbit 		= 0,
            _S_badbit 		= 1L << 0,
            _S_eofbit 		= 1L << 1,
            _S_failbit		= 1L << 2,
            _S_ios_iostate_end = 1L << 16,
            _S_ios_iostate_max = __INT_MAX__,
            _S_ios_iostate_min = ~__INT_MAX__
            */

            // what should happen here?
            // should the badbit be set when send returns false?
            setstate(badbit);
            setp(data+written, data+IEC_BUFFER_SIZE); // set pbase to point to next unwritten char
            return written;
        }
        else {
            // ATN was pulled
            setp(data+written, data+IEC_BUFFER_SIZE); // set pbase to point to next unwritten char
            return written;
        }
    }

    char lastChar = *(pptr()); // ok, this should be the last char, left out from the above loop
    Debug_printv("lastChar[%.2X]", lastChar);

    if(!lastOne && !eof()) {
        // probably more bytes to come, so
        // here we wrote all buffer chars but the last one.
        // we will take this last byte, put it at position 0 and set pptr to 1
        data[0] = lastChar; // let's put it at position 0
        setp(data, data+IEC_BUFFER_SIZE); // reset the beginning and ending buffer pointers
        pbump(1); // and set pptr to 1 to tell there's 1 byte in our buffer
        Debug_printv("IEC easyWrite writes leaving one char behind: data0[%.2X]", data[0]);
    }
    else {
        // ok, so we have last character, signal it
        Debug_printv("IEC easyWrite writes THE LAST ONE with EOI: [%.2X]", lastChar);
        m_iec->sendEOI(lastChar);
        setp(data, data+IEC_BUFFER_SIZE);
    }

    return written;
}

int oiecstream::overflow(int ch) {
    Debug_printv("overflow for iec called");
    char* end = pptr();
    if ( ch != EOF ) {
        *end ++ = ch;
    }

    auto written = easyWrite(false);

    if ( written == 0 ) {
        ch = EOF;
    } else if ( ch == EOF ) {
        ch = 0;
    }
    
    return ch;
};

int oiecstream::sync() { 
    if(pptr() == pbase()) {
        Debug_printv("sync for iec called - no more data");
        return 0;
    }
    else {
        Debug_printv("sync for iec called - will write last remaining char if available");
        auto result = easyWrite(true); 
        return (result != 0) ? 0 : -1;  
    }  
};


/********************************************************
 * oiecstream
 * 
 * Standard C++ stream for writing to IEC
 ********************************************************/

void oiecstream::putUtf8(U8Char* codePoint) {
    //Serial.printf("%c",codePoint->toPetscii());
    //Debug_printv("oiecstream calling put");
    auto c = codePoint->toPetscii();
    put(codePoint->toPetscii());        
}

    // void oiecstream::writeLn(std::string line) {
    //     // line is utf-8, convert to petscii

    //     std::string converted;
    //     std::stringstream ss(line);

    //     while(!ss.eof()) {
    //         U8Char codePoint(&ss);
    //         converted+=codePoint.toPetscii();
    //     }

    //     Debug_printv("UTF8 converted to PETSCII:%s",converted.c_str());

    //     (*this) << converted;
    // }
