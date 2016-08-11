//
//  WavFile.cpp
//  WavFileOpener
//
//  Created by John Asper on 2016/8/10.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include "WavFile.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

// Default Constructor
WavFile::WavFile(){
    samples = NULL;
    format = 0;
    num_channels = 0;
    sample_rate = 0;
    byte_rate = 0;
    block_align = 0;
    bits_per_sample = 0;
}

// Constructor
// Loads specified wav file into memory
WavFile::WavFile(std::string filename){
    open(filename);
}

// Destructor
// Automatically deallocates any allocated memory
WavFile::~WavFile(){
    if(samples){
        for (int i = 0; i < num_channels; ++i) {
            delete [] samples[i];
        }
        delete [] samples;
    }
}

enum class WavChunks{
    RiffHeader = 0x52494646,
    Format = 0x666D7420,
    Data = 0x64617461
};

enum class WavFormat {
    PulseCodeModulation = 0x01,
    IEEEFloatingPoint = 0x03,
    ALaw = 0x06,
    MuLaw = 0x07,
    IMAADPCM = 0x11,
    YamahaITUG723ADPCM = 0x16,
    GSM610 = 0x31,
    ITUG721ADPCM = 0x40,
    MPEG = 0x50,
    Extensible = 0xFFFE
};


const unsigned char KSDATAFORMAT_SUBTYPE_PCM[] = {
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x10,
    0x00,
    0x80,
    0x00,
    0x00,
    0xaa,
    0x00,
    0x38,
    0x9b,
    0x71};

bool compareSubtype(const unsigned char a[16], const unsigned char b[16]){
    for(int i = 0; i < 16; ++i){
        if(a[i] != b[i])
            return false;
    }
    return true;
}


// Normalizes the samples over the entire file
// sample/max_sample for all samples
void WavFile::normalizeSamples(){
    float max_sample = 0;
    
    for(int channel = 0; channel < num_channels; ++channel){
        for(int sample = 0; sample < num_samples; ++sample){
            if(max_sample < std::abs(samples[channel][sample])){
                max_sample = std::abs(samples[channel][sample]);
            }
        }
    }
    
    std::cout << "Max Sample = " << std::setprecision(10) << max_sample << ", normalizing..." << std::endl << std::endl;
    
    for(int channel = 0; channel < num_channels; ++channel){
        for(int sample = 0; sample < num_samples; ++sample){
            samples[channel][sample] /= max_sample;
        }
    }
}

// Turns a 3 byte char array into a 32 bit int
// The ternary operator decides if sign extension is necessary
inline int32_t int24to32(unsigned char *in){
    return ((in[2] & 0x80) ? (0xff <<24) : 0) | (in[2] << 16) | (in[1] << 8) | in[0];
}

// Normalizing factors for conversions
const float uint8normalize = 1.0f/0xff;
const float int16normalize = 1.0f/0x7fff;
const float int24normalize = 1.0f / 8388607.0; // Magic number, maps smallest to -1 and largest to 1

// Open a new wav file
// Deallocates old file if necessary
void WavFile::open(std::string filename){
    std::ifstream f;
    f.open(filename, std::ios::binary);
    if(!f.is_open()){
        std::cerr << "Error: " << strerror(errno) << std::endl;
        throw std::runtime_error("WavFile Error: Could not open file\n");
    }
    
    while(!f.eof()){
        uint32_t chunkid;
        f.read(reinterpret_cast<char*>(&chunkid), sizeof(chunkid));
        chunkid = __builtin_bswap32(chunkid);
        switch((WavChunks)chunkid){
                
            case WavChunks::RiffHeader:
                f.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
                uint32_t format_specifier;
                f.read(reinterpret_cast<char*>(&format_specifier), sizeof(filesize));
                if (__builtin_bswap32(format_specifier) != 0x57415645) {
                    throw std::runtime_error("WavFile Error: Not a Wave File!");
                }
                break;
                
            case WavChunks::Format:
                uint32_t chunksize;
                f.read(reinterpret_cast<char*>(&chunksize), sizeof(chunksize));;
                
                f.read(reinterpret_cast<char*>(&format), sizeof(format));
                
                f.read(reinterpret_cast<char*>(&num_channels), sizeof(num_channels));
                f.read(reinterpret_cast<char*>(&sample_rate), sizeof(sample_rate));
                f.read(reinterpret_cast<char*>(&byte_rate), sizeof(byte_rate));
                f.read(reinterpret_cast<char*>(&block_align), sizeof(block_align));
                f.read(reinterpret_cast<char*>(&bits_per_sample), sizeof(bits_per_sample));
                
                if ((WavFormat)format != WavFormat::PulseCodeModulation){
                    uint16_t extra_params_size;
                    f.read(reinterpret_cast<char*>(&extra_params_size), sizeof(extra_params_size));
                    uint16_t valid_bits_per_sample;
                    f.read(reinterpret_cast<char*>(&valid_bits_per_sample), sizeof(valid_bits_per_sample));
                    uint32_t channel_mask;
                    f.read(reinterpret_cast<char*>(&channel_mask), sizeof(channel_mask));
                    unsigned char subformat[16];
                    f.read((char*)subformat, 16);
                    
                    if(compareSubtype(subformat, KSDATAFORMAT_SUBTYPE_PCM)){
                        std::cout << "Subformat is KSDATAFORMAT_SUBTYPE_PCM" << std::endl;
                    }
                }
                
                break;
                
            case WavChunks::Data:
                uint32_t datasize;
                f.read(reinterpret_cast<char*>(&datasize), sizeof(datasize));
                samples = new float*[num_channels];
                num_samples =datasize*8/num_channels/bits_per_sample; // calculate number of samples
                
                for (int channel = 0; channel < num_channels; ++channel) {
                    samples[channel] = new float[num_samples];
                }
                
                for (int sample = 0; sample < num_samples; ++sample) {
                    for(int channel = 0; channel < num_channels; ++channel){
                        if (bits_per_sample == 8) {
                            uint8_t temp8bit;
                            f.read(reinterpret_cast<char*>(&temp8bit), sizeof(temp8bit));
                            samples[channel][sample] = uint8normalize*(float)temp8bit;
                        } else if (bits_per_sample == 16) {
                            int16_t temp16bit;
                            f.read(reinterpret_cast<char*>(&temp16bit), sizeof(temp16bit));
                            samples[channel][sample] = int16normalize*(float)temp16bit;
                        } else if (bits_per_sample == 24) {
                            unsigned char temp24bit[3] = {0,0,0};
                            f.read((char*)temp24bit, 3); // Uh Oh, magic numbers! 24 bits = 3 bytes
                            int32_t temp = int24to32(temp24bit);
                            samples[channel][sample] = int24normalize*(float)temp;
                        }
                    }
                }
                break;
                
            default:
                unsigned char tag[4];
                tag[0] = (chunkid >> 24) & 0xff;
                tag[1] = (chunkid >> 16) & 0xff;
                tag[2] = (chunkid >> 8) & 0xff;
                tag[3] = chunkid & 0xff;
                std::cout << "Encountered unknown chunk, ID: " << tag << " or " << std::hex << chunkid;
                std::cout << std::dec << " at byte " << f.tellg() << std::endl << std::endl;
                uint32_t skipsize;
                f.read(reinterpret_cast<char*>(&skipsize), sizeof(skipsize));
                f.ignore(static_cast<int>(skipsize));
        }
    }
}

// Getters
uint16_t WavFile::getFormat(){
    return format;
}

uint16_t WavFile::getNumChannels(){
    return num_channels;
}

uint32_t WavFile::getSampleRate(){
    return sample_rate;
}

uint32_t WavFile::getByteRate(){
    return byte_rate;
}

uint16_t WavFile::getBlockAlign(){
    return block_align;
}

uint16_t WavFile::getBitsPerSample(){
    return bits_per_sample;
}

uint32_t WavFile::getNumSamples(){
    return num_samples;
}

float ** WavFile::getData(){
    return samples;
}

std::string audioFormatToString(WavFormat n){
    switch (n) {
        case WavFormat::PulseCodeModulation:
            return std::string("Linear PCM");
            break;
        case WavFormat::IEEEFloatingPoint:
            return std::string("IEEEFloating Point");
            break;
        case WavFormat::ALaw:
            return std::string("ALaw");
            break;
        case WavFormat::MuLaw:
            return std::string("MuLaw");
            break;
        case WavFormat::IMAADPCM:
            return std::string("IMAAD PCM");
            break;
        case WavFormat::YamahaITUG723ADPCM:
            return std::string("Yamaha ITUG723AD PCM");
            break;
        case WavFormat::GSM610:
            return std::string("GSM 610");
            break;
        case WavFormat::ITUG721ADPCM:
            return std::string("ITUG721AD PCM");
            break;
        case WavFormat::MPEG:
            return std::string("MPEG");
            break;
        case WavFormat::Extensible:
            return std::string("Extensible");
        default:
            return std::string("Unknown");
            break;
    }
}

std::string WavFile::toString(){
    std::stringstream s;
    s << "-Wave File-" << std::endl;
    s << "\tSample Rate = " << sample_rate << " Hz" << std::endl;
    s << "\tAudio Format = " << audioFormatToString((WavFormat)format) << std::endl;
    s << "\tNumber of Channels = " << num_channels << std::endl;
    s << "\tByte Rate = " << byte_rate << std::endl;
    s << "\tBlock Align = " << block_align << std::endl;
    s << "\tBits per Sample = " << bits_per_sample << std::endl;
    s << "\tNumber of Samples = " << num_samples << std::endl << std::endl;
    return s.str();
}