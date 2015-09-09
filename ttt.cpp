// just in case

#include <iostream>
#include <fstream>
#include <cstdint>
#include <thread>

#include "boost/filesystem.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/strand.hpp"
#include "boost/crc.hpp"

namespace ba = boost::asio;
namespace fs = boost::filesystem;

typedef boost::crc_32_type::value_type crc_type;
const size_t block_size = 1024 << 10;
const size_t crc_size   = sizeof(crc_type);

typedef std::vector<char> buffer_type;
typedef std::shared_ptr<std::ifstream> ifstream_sptr;
typedef std::shared_ptr<std::ofstream> ofstream_sptr;

class worker {

    ba::io_service::strand &disp_;
    ba::io_service::strand &dispw_;
    buffer_type             buff_;
    ifstream_sptr           is_;
    ofstream_sptr           os_;

public:

    worker( ba::io_service::strand &disp, ba::io_service::strand &wdisp,
            size_t block_size, ifstream_sptr is, ofstream_sptr os )
        :disp_(disp)
        ,dispw_(wdisp)
        ,buff_(block_size)
        ,is_(is)
        ,os_(os)
    { }

    void start( )
    {
        disp_.post( [this]( ){ read_handler( ); } );
    }

    ba::io_service &get_io_service( )
    {
        return disp_.get_io_service( );
    }

private:

    void write_crc( crc_type crc, size_t pos )
    {
        os_->seekp( pos );
        os_->write( reinterpret_cast<const char *>(&crc), sizeof(crc) );
    }

    void read_handler( )
    {
        size_t current_read_pos = is_->tellg( );
        size_t current_crc_pos = (current_read_pos / buff_.size( )) * crc_size;
        auto read = is_->read( buff_.data( ), buff_.size( ) ).gcount( );
        if( read == 0 ) {
            std::cout << "Final!\n";
            return;
        }
        current_read_pos += read;
        std::cout << current_read_pos << " " << read << "\n";
        disp_.get_io_service( ).post( [this, read, current_crc_pos](  ) { 
            work_handler( read, current_crc_pos ); 
        } );
    }

    void work_handler( size_t length, size_t crc_position )
    {
        boost::crc_32_type res;
        res.process_bytes( buff_.data( ), length );
        auto crc = res.checksum( );

        dispw_.post( [this, crc, crc_position ] { 
            write_crc( crc, crc_position ); 
        } );

        disp_.post( [this, crc, crc_position ] { 
            read_handler( ); 
        } );
    }

};

void attach( worker &wrk )
{
    try {
        wrk.start( );
        while( wrk.get_io_service( ).run_one( ) );
    } catch( const std::exception &ex ) {
        std::cout << "Thread failed: " << ex.what( ) << "\nStop all works.\n";
        wrk.get_io_service( ).stop( );
        return;
    }
    std::cout << "Thread exit.\n";
}
  
int main( int argc, const char *argv[] )
{
    if( argc < 2 ) {
        std::cout << "Need!\n";
        return 1;
    }

    try {

        std::string inp = argv[1];
        std::string otp = inp + ".crc";

        ba::io_service          ios;
        ba::io_service::strand  dispatcher(ios);
        ba::io_service::strand  wdispatcher(ios);
        
        auto insize = fs::file_size( inp );
        auto opsize = (insize / block_size + (insize % block_size ? 1 : 0)) * crc_size;
 
        auto is = std::make_shared<std::ifstream>( inp, std::ios::in  | std::ios::binary );
        auto os = std::make_shared<std::ofstream>( otp, std::ios::out | std::ios::binary );

        auto threads = std::thread::hardware_concurrency( );

        std::vector<worker> storage;

        for( size_t i = 0; i<threads; ++i ) {
            storage.emplace_back( dispatcher, wdispatcher, block_size, is, os );
        }

        std::vector<std::thread> thread_vector;
        for( size_t i = 1; i<threads; ++i ) {
            thread_vector.emplace_back( attach, std::ref( storage[i] ) );
        }        

        attach( storage[0] );
        
        for( auto &t: thread_vector ) {
            if( t.joinable( ) ) {
                t.join( );
            }
        }

    } catch( const std::exception &ex ) {
        std::cerr << "failed to work: " << ex.what( );
    }

    return 0;
}
