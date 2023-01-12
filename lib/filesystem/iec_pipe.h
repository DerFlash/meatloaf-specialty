#ifndef MEATLIB_FILESYSTEM_IEC_PIPE
#define MEATLIB_FILESYSTEM_IEC_PIPE

#include "meat_buffer.h"
#include "wrappers/iec_buffer.h"

/*
This is the main IEC named channel abstraction. 

- each named channel is connected to one file

- it allows pusing data to/from remote file

- when ATN is received the transfer stops and will be seamlesly resumed on next call to send/receive

- resuming transfer is possible as long as this channel is open
*/
class iecPipe {
    Meat::iostream* fileStream = nullptr;
    oiecstream iecStream;
    iecBus* m_iec;
    std::ios_base::openmode mode;

public:
    bool establish(std::string filename, std::ios_base::openmode m, iecBus* i) {
        if(fileStream != nullptr)
            return false;

        mode = m;
        m_iec = i;
        if(mode == std::ios_base::openmode::_S_in) // we'll be reading the file, so lets use the smart buffer for easy EOI handling
            iecStream.open(m_iec);

        fileStream = new Meat::iostream(filename.c_str(), mode);
    }

    // this stream is disposed of, nothing more will happen with it!
    // this must be closed when actually talking to this stream, as it will send EOI
    void finish() {
        if(fileStream != nullptr)
            delete fileStream;

        fileStream = nullptr;
        iecStream.close(); // will push last byte and send EOI
    }

    // push file bytes to C64
    void fileToIec() {
        if(fileStream == nullptr || mode != std::ios_base::openmode::_S_in)
            return;
        
        int i = 0;

        while( !fileStream->eof() && fileStream->bad() == 0 && iecStream.bad() == 0 )
        {
            char nextChar;

            // NOTE: ATTN doesn't stop us from writing to the IEC smart buffer (if this is a big buffer, not single-byte buffer),
            // as we can continue sending the buffer data when we're allowed to talk again. We're just caching a few next bytes of
            // the file in the smart buffer
            // NOTE 2: This will be obviously wrong when seeking! So we have to clean the buffer on seek!
            (*fileStream).checkNda();
            if((*fileStream).nda()) {
                // OK, Jaimme. So you told me there's a way to signal "no data available on IEC", here's the place
                // to send it:
            }
            else if(!(*fileStream).eof()) {
                (*fileStream).get(nextChar);
                iecStream.write(&nextChar, 1);
            }

            // Toggle LED
            // if (i % 50 == 0)
            // {
            // 	fnLedManager.toggle(eLed::LED_BUS);
            // }

            // if ( i % 8 == 0)
            // 	Debug_println("");

            Debug_printf("%.2X ", nextChar);

            i++;	
        }
    }

    // push C64 bytes to the file
    void iecToFile() {
        if(fileStream == nullptr || mode != std::ios_base::openmode::_S_out)
            return;
        
        // NOTE: 
        while( !fileStream->eof() && fileStream->bad() == 0)
        {
            char nextChar;

            int16_t byteToSave = m_iec->receive(); // -1 == error
            (*fileStream).put('a');
        }
    }
};

#endif /* MEATLIB_FILESYSTEM_IEC_PIPE */
