//
//  WavFile.cpp
//  WavFileOpener
//
//  Created by John Asper on 2016/8/10.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include "WavFile.hpp"
#include <fstream>

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

// Open a new wav file
// Deallocates old file if necessary
void WavFile::open(std::string filename){
    std::ifstream f;
    f.open(filename, std::ios::binary | std::ios::ate);
    if(!f.is_open()){
        std::cerr << "Error: " << strerror(errno) << std::endl;
        throw std::runtime_error("WavFile Error: Could not open file\n");
    }
    
    while(!f.eof()){
        uint32_t chunkid;
        f.read(reinterpret_cast<char*>(&chunkid), sizeof(chunkid));
        switch((WavChunks)chunkid){
                
            case WavChunks::RiffHeader:
                uint32_t filesize;
                f.read(reinterpret_cast<char*>(filesize), sizeof(filesize));
                uint32_t format_specifier;
                f >> format_specifier;
                if (format_specifier != 0x57415645) {
                    throw std::runtime_error("WavFile Error: Not a Wave File!");
                }
                break;
                
            case WavChunks::Format:
                uint32_t chunksize;
                f >> chunksize;
                f >> format;
                
                if ((WavFormat)format != WavFormat::PulseCodeModulation){
                    throw std::runtime_error("WavFile Error: only PCM wave files are supported!");
                }
                
                f >> num_channels;
                f >> sample_rate;
                f >> byte_rate;
                f >> block_align;
                f >> bits_per_sample;
                
                break;
                
            case WavChunks::Data:
                uint32_t datasize;
                f >> datasize;
                samples = new uint16_t*[num_channels];
                num_samples =datasize*8/num_channels/bits_per_sample; // calculate number of samples
                
                uint8_t temp8bit;
                int16_t temp16bit;
                
                for (int channel = 0; channel < num_channels; ++channel){
                    samples[channel] = new uint16_t[num_samples];
                    
                    for (int sample = 0; sample < num_samples; ++sample){
                        if (bits_per_sample == 8) {
                            f >> temp8bit;
                            samples[channel][sample] = (uint16_t)temp8bit;
                        } else if (bits_per_sample == 16) {
                            f >> temp16bit;
                            samples[channel][sample] = temp16bit;
                        }
                    }
                }
                break;
                
            default:
                uint32_t skipsize;
                f >> skipsize;
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

uint16_t ** WavFile::getData(){
    return samples;
}
