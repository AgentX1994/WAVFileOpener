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
    
    std::cout << "Max Sample = " << max_sample << ", normalizing..." << std::endl << std::endl;
    
    for(int channel = 0; channel < num_channels; ++channel){
        for(int sample = 0; sample < num_samples; ++sample){
            samples[channel][sample] /= max_sample;
        }
    }
}

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
                uint32_t filesize;
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
                
                if ((WavFormat)format != WavFormat::PulseCodeModulation){
                    throw std::runtime_error("WavFile Error: only PCM wave files are supported!");
                }
                
                f.read(reinterpret_cast<char*>(&num_channels), sizeof(num_channels));
                f.read(reinterpret_cast<char*>(&sample_rate), sizeof(sample_rate));
                f.read(reinterpret_cast<char*>(&byte_rate), sizeof(byte_rate));
                f.read(reinterpret_cast<char*>(&block_align), sizeof(block_align));
                f.read(reinterpret_cast<char*>(&bits_per_sample), sizeof(bits_per_sample));
                
                break;
                
            case WavChunks::Data:
                uint32_t datasize;
                f.read(reinterpret_cast<char*>(&datasize), sizeof(datasize));
                samples = new float*[num_channels];
                num_samples =datasize*8/num_channels/bits_per_sample; // calculate number of samples
                
                uint8_t temp8bit;
                int16_t temp16bit;
                
                for (int channel = 0; channel < num_channels; ++channel) {
                    samples[channel] = new float[num_samples];
                }
                
                for (int sample = 0; sample < num_samples; ++sample) {
                    for(int channel = 0; channel < num_channels; ++channel){
                        if (bits_per_sample == 8) {
                            f.read(reinterpret_cast<char*>(&temp8bit), sizeof(temp8bit));
                            samples[channel][sample] = (float)temp8bit;
                        } else if (bits_per_sample == 16) {
                            f.read(reinterpret_cast<char*>(&temp16bit), sizeof(temp16bit));
                            samples[channel][sample] = (float)temp16bit;
                        }
                    }
                }
                break;
                
            default:
                uint32_t skipsize;
                f.read(reinterpret_cast<char*>(&skipsize), sizeof(skipsize));
                f.ignore(static_cast<int>(skipsize));
        }
    }
    
    normalizeSamples();
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