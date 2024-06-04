#ifndef WATERFALLVIEW_NEW_H
#define WATERFALLVIEW_NEW_H

#include <QWidget>
#include <QPainter>
#include <QThread>
#include <QImage>
#include <QResizeEvent>
#include <QColorSpace>

#include <fstream>
#include <iostream>
#include <queue>
#include <complex>
#include <vector>


#include "fft2.hpp"
#include "fftwindows.hpp"
#include "tools.hpp"




//class ThreadFileReader : public QThread {
//    Q_OBJECT

//    std::atomic_bool _is_break, _is_running;
//    uint64_t _file_byte_size;

//    std::ifstream _file;

//    std::vector<std::map<int, std::vector<char>>> _queue;

//    void run() override {
//        while( _is_running) {

//            while( _is_break);
//        }
//    }

//    /// @brief Reads Filesize in Bytes
//    void readFileSize() {
//        uint64_t old_pos = _file.tellg();
//        _file.seekg( 0, std::ios::end);
//        _file_byte_size = _file.tellg();
//        _file.seekg( old_pos);
//    }

//public:
//    FileReaderForThreads( const std::string &file, uint64_t thread_cnt, uint64_t read_size) :
//        _is_break( false), _byte_size( 0 ), _queue_size( 1024), _is_running( false) {
//        _file.open( file, std::ios::binary | std::ios::in);
//        if( ! _file.is_open())
//            throw std::runtime_error( "FEHLER std::ifstream::open()");
//        readFileSize();

//    }
//    ~FileReaderForThreads() {
//        _file.close();
//    }

//};




/// @brief Thread based7 runner for large data background processing
class ImageProcessor : public QThread{
    Q_OBJECT

    FFT2 *_fft;
    std::atomic_bool _running;
    QImage _image;
    std::vector<std::complex<float>> _in_buffer;
    std::vector<std::complex<float>> _data_buffer;
    std::fstream _file;
    uint64_t _chunk_leng;

    void run() override {
        while( _running) {
            _file.read( reinterpret_cast<char*>( _in_buffer.data()), _chunk_leng);
            // check if enough samples to perform full fft
            if( _file.gcount() != _chunk_leng) {
                _running = false;
                break;
            }

            _fft->fft( _in_buffer, _data_buffer);
            //... edit  in QImage
        }
    }

public:
    ImageProcessor( QImage image, std::string path, uint64_t fft_exp) : _image( image) {
        _fft = new FFT2( fft_exp);
        _file.open( path);
        _in_buffer.resize( _fft->leng());
        _data_buffer.resize( _fft->leng());
    }
    void stop() { _running = false;}
    void start() { _running = true;}

    void restart( QImage image) {
        _image = image;
    }

signals:
    /// @brief Indicates for another update
    void nextReady();
    /// @brief maybe whole file read
    void finished();
};

//-----------------------------------------------------------------------------


/// @brief Ordinary Constructor for showview
class Waterfallview : public QWidget {
    Q_OBJECT

    uint64_t _current_line;

    uint64_t _file_byte_size;
    QPainter* _painter;
    QImage _image;
    ImageProcessor *_processor;

    std::vector<std::complex<float>> _buffer, _buffer_fft, _buffer_fft_abs;
    FFT2* _fft;

    std::vector<std::ifstream> _file_reader;
    int _mode;
    uint64_t _pos_overlap;

    void resizeEvent( QResizeEvent *re) override {
        _image = QImage( this->size(), QImage::Format_Grayscale8);
        _processor->restart( _image);


    }
    void paintEvent( QPaintEvent *qpe) override {
        _painter->drawImage( 0, 0, _image);
    }

    /// @brief Fuegt eine FFT Linie hinzu
    /// @param input fft magnitudes in 10*log10
    void
    addLineToImage( const std::vector<float> &input) {

        for( uint64_t w = 0; w < input.size(); )
            _image.setPixel( _current_line, w,  static_cast<uint>(input.at( w) ));
    }

    //    /// @brief Reads Filesize in Bytes
    //    void readFileSize() {
    //        uint64_t old_pos = _file.tellg();
    //        _file.seekg( 0, std::ios::end);
    //        _file_byte_size = _file.tellg();
    //        _file.seekg( old_pos);
    //    }

public:
    Waterfallview( uint64_t fft_exp = 8) : _file_byte_size( 0) {
        _painter = new QPainter( this);
        _image = QImage( this->size(), QImage::Format_Grayscale8);
        setFFTExp( fft_exp);

        _image.setColorSpace( QColorSpace::SRgb);
    }
    ~Waterfallview() {
        delete _painter;
    }

    // QImage Groesse doppelte
    void
    setImageSize( uint64_t w, uint64_t h) {
        _image = _image.scaled( w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation;)
    }

    // Reset der aktuellen fft
    void
    setFFTExp( const uint64_t leng) {
        if( leng != Tools::nextPow2( leng))
            throw std::invalid_argument("FEHLER setFFTExp() leng not a power of two");
        if( _fft) delete _fft;
        _fft = new FFT2( leng);
        // Start des letzten viertels
        _buffer.reserve( _fft->leng());
        _buffer_fft.resize( _fft->leng());
        _buffer_fft_abs.resize( _fft->leng());
    }

    //    void
    //    setMode( int mode_nr = 0) {
    //        _mode = mode_nr;
    //    }

    /// @brief Eingangsdaten buffern, volle ffts rausziehen und Waterfall updaten
    void
    addData( const std::vector<std::complex<float>> &input) {
        // neue Daten an den internen Puffer anhanegen
        for( auto &data : input)
            _buffer.push_back( data);

        uint64_t consumed = 0;
        // fft as long as enough data available with respect to the overlap
        while(( _buffer.size() - consumed) >= _fft->leng()) {

            _fft->fft( _buffer.data() + cnt * _fft->leng(), _buffer_fft.data());
            std::transform( _buffer_fft.begin(), _buffer_fft.end(), _buffer_fft_abs.begin(),
                           [] (std::complex<float> &val) { return 10*std::log10( std::abs( val));});
            addLineToImage(  _buffer_fft_abs);
            consumed += _fft->leng


        }
        // than update gui
        update();
    }


public slots:

};

#endif // WATERFALLVIEW_NEW_H
