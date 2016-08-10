//
//  WavFile.hpp
//  WavFileOpener
//
//  Created by John Asper on 2016/8/10.
//  Copyright © 2016 John Asper. All rights reserved.
//

#ifndef WavFile_hpp
#define WavFile_hpp

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstdint>

/* WavFile class
 * 
 * Represents a WavFile loaded into memory
 */
class WavFile {
public:
    
    // Default Constructor
    WavFile();
    
    // Constructor
    // Loads specified wav file into memory
    WavFile(std::string filename);
    
    // Destructor
    // Automatically deallocates any allocated memory
    ~WavFile();
    
    // Open a new wav file
    // Deallocates old file if necessary
    void open(std::string filename);
    
    // Getters
    uint16_t getFormat();
    uint16_t getNumChannels();
    uint32_t getSampleRate();
    uint32_t getByteRate();
    uint16_t getBlockAlign();
    uint16_t getBitsPerSample();
    uint32_t getNumSamples();
    uint16_t ** getData();
    
    // Operator to access individual channels
    uint16_t *operator[](int index){
        if(index < 0 || index >= num_channels){
            throw std::out_of_range("Tried to access a channel that doesn't exist!");
        } else {
            return samples[index];
        }
    }
    
protected:
private:
    uint16_t format; // Currently only supports 1 (PCM)
    uint16_t num_channels; // Number of audio channels;
    uint32_t sample_rate; // Sample rate of the audio;
    uint32_t byte_rate; // bytes per second of the audio;
    uint16_t block_align; // Alignment of blocks in the data stream
    uint16_t bits_per_sample; // Number of bits per sample;
    
    uint32_t num_samples;
    uint16_t **samples; // The sample arrays, an array of floats for each channel
};

#endif /* WavFile_hpp */
