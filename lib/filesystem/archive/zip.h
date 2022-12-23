
#include <iostream>

#include <archive_reader.hpp>
#include <archive_exception.hpp>

void test( void )
{
    // try
    {
        namespace ar = ns_archive::ns_reader;
        std::fstream fs("some_tar_file.tar");
        ns_archive::reader reader = ns_archive::reader::make_reader<ar::format::_ALL, ar::filter::_ALL>(fs, 10240);

        for(auto entry : reader)
        {
            // get file name
            std::cout << entry->get_header_value_pathname() << std::endl;
            // get file content
            std::cout << entry->get_stream().rdbuf() << std::endl << std::endl;
        }
    }
    // catch(ns_archive::archive_exception& e)
    // {
    //     std::cout << e.what() << std::endl;
    // }
}