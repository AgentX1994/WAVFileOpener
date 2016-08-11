//
//  main.cpp
//  WavFileOpener
//
//  Created by John Asper on 2016/8/10.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <AudioToolbox/AudioToolbox.h>

#include "WavFile.hpp"

typedef struct {
    WavFile *w;
    uint32_t cur_sample;
    uint32_t num_samples;
    uint32_t packets_per_read;
    AudioStreamBasicDescription fmt;
} Player;

void AQcallback (void *ptr, AudioQueueRef queue, AudioQueueBufferRef buf_ref){
    Player *p = (Player *)ptr;
    OSStatus status;
    AudioQueueBuffer *buf = buf_ref;
    uint32_t nsamples = buf->mAudioDataByteSize / p->fmt.mBytesPerPacket;
    float *samp = (float *)buf->mAudioData;
        
    if(p->cur_sample > p->num_samples){
        AudioQueueStop(queue, false);
        return;
    }
    
    int sample = 0;
    while(sample < nsamples * p->w->getNumChannels()){
        for (int channel = 0; channel < p->w->getNumChannels(); ++channel) {
            if (p->cur_sample > p->num_samples) {
                samp[sample] = 0;
            } else {
                samp[sample] = p->w->getData()[channel][p->cur_sample];
            }
            ++sample;
        }
        ++(p->cur_sample);
    }
    status = AudioQueueEnqueueBuffer (queue, buf_ref, 0, NULL);
}

std::string getError(OSStatus status){
    switch (status) {
        case kAudioQueueErr_InvalidBuffer:
            return std::string("kAudioQueueErr_InvalidBuffer");
        case kAudioQueueErr_BufferEmpty:
            return std::string("kAudioQueueErr_BufferEmpty");
        case kAudioQueueErr_DisposalPending:
            return std::string("kAudioQueueErr_DisposalPending");
        case kAudioQueueErr_InvalidPropertySize:
            return std::string("kAudioQueueErr_InvalidPropertySize");
        case kAudioQueueErr_InvalidParameter:
            return std::string("kAudioQueueErr_InvalidParameter");
        case kAudioQueueErr_CannotStart:
            return std::string("kAudioQueueErr_CannotStart");
        case kAudioQueueErr_InvalidDevice:
            return std::string("kAudioQueueErr_InvalidDevice");
        case kAudioQueueErr_BufferInQueue:
            return std::string("kAudioQueueErr_BufferInQueue");
        case kAudioQueueErr_InvalidRunState:
            return std::string("kAudioQueueErr_InvalidRunState");
        case kAudioQueueErr_InvalidQueueType:
            return std::string("kAudioQueueErr_InvalidQueueType");
        case kAudioQueueErr_Permissions:
            return std::string("kAudioQueueErr_Permissions");
        case kAudioQueueErr_InvalidPropertyValue:
            return std::string("kAudioQueueErr_InvalidPropertyValue");
        case kAudioQueueErr_PrimeTimedOut:
            return std::string("kAudioQueueErr_PrimeTimedOut");
        case kAudioQueueErr_CodecNotFound:
            return std::string("kAudioQueueErr_CodecNotFound");
        case kAudioQueueErr_InvalidCodecAccess:
            return std::string("kAudioQueueErr_InvalidCodecAccess");
        case kAudioQueueErr_QueueInvalidated:
            return std::string("kAudioQueueErr_QueueInvalidated");
        case kAudioQueueErr_RecordUnderrun:
            return std::string("kAudioQueueErr_RecordUnderrun");
        case kAudioQueueErr_EnqueueDuringReset:
            return std::string("kAudioQueueErr_EnqueueDuringReset");
        case kAudioQueueErr_InvalidOfflineMode:
            return std::string("kAudioQueueErr_InvalidOfflineMode");
        case kAudioFormatUnsupportedDataFormatError:
            return std::string("kAudioFormatUnsupportedDataFormatError");
        default:
            return std::string("No Error or Unknown Error");
    }

}

void DeriveBufferSize (
                       AudioStreamBasicDescription &ASBDesc,
                       UInt32                      maxPacketSize,
                       Float64                     seconds,
                       UInt32                      *outBufferSize,
                       UInt32                      *outNumPacketsToRead
) {
    static const int maxBufferSize = 0x50000;
    static const int minBufferSize = 0x4000;
    
    if (ASBDesc.mFramesPerPacket != 0) {
        Float64 numPacketsForTime =
        ASBDesc.mSampleRate / ASBDesc.mFramesPerPacket * seconds;
        *outBufferSize = numPacketsForTime * maxPacketSize;
    } else {
        *outBufferSize =
        maxBufferSize > maxPacketSize ?
        maxBufferSize : maxPacketSize;
    }
    
    if (
        *outBufferSize > maxBufferSize &&
        *outBufferSize > maxPacketSize
        )
        *outBufferSize = maxBufferSize;
    else {
        if (*outBufferSize < minBufferSize)
            *outBufferSize = minBufferSize;
    }
    
    *outNumPacketsToRead = *outBufferSize / maxPacketSize;
}

static const float time_per_loop = .25f;

void printChannelsToCSV(WavFile &w){
    std::ofstream out;
    out.open("/Users/john/Documents/Xcode Projects/WavFileOpener/test.csv", std::ios::out | std::ios::trunc);
    if(!out.is_open()){
        throw std::runtime_error("COULD NOT OPEN FILE!");
    }
    for(int channel = 0; channel < w.getNumChannels(); ++channel){
        for(int sample = 0; sample < w.getNumSamples(); ++sample){
            out << w[channel][sample];
            if (sample == w.getNumSamples() - 1) {
                out << std::endl;
            } else {
                out << ", ";
            }
        }
    }
    out.close();
}

int main(int argc, const char * argv[]) {
    WavFile wav ("/Users/john/Documents/Xcode Projects/WavFileOpener/test.wav");
    std::cout << wav.toString();
    printChannelsToCSV(wav);
    
    Player p = {&wav, 0, wav.getNumSamples(), 0, {0}};
    
    AudioQueueRef queue;
    OSStatus status;
    
    p.fmt.mSampleRate = wav.getSampleRate();
    p.fmt.mFormatID = kAudioFormatLinearPCM;
    p.fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    p.fmt.mFramesPerPacket = 1;
    p.fmt.mChannelsPerFrame = wav.getNumChannels();
    p.fmt.mBytesPerPacket = p.fmt.mBytesPerFrame = sizeof(float)*wav.getNumChannels();
    p.fmt.mBitsPerChannel = sizeof(float)*8;
    
    status = AudioQueueNewOutput(&(p.fmt), AQcallback, &p, CFRunLoopGetCurrent(),
                                 kCFRunLoopCommonModes, 0, &queue);
    
    std::cout << "New Output Status: " << getError(status) << std::endl;
    
    const int num_bufs = 4;
    uint32_t buffer_size;
    AudioQueueBufferRef buf_refs[num_bufs];
    AudioQueueBuffer *bufs[num_bufs];
    
    DeriveBufferSize(p.fmt, p.fmt.mBytesPerPacket, time_per_loop, &buffer_size, &p.packets_per_read);
    std::cout << "Buffer Size = " << buffer_size << " bytes, reading " << p.packets_per_read << " packets per callback" << std::endl << std::endl;
    
    for(int i = 0; i < num_bufs; ++i){
        AudioQueueAllocateBuffer(queue, buffer_size, &(buf_refs[i]));
        bufs[i] = buf_refs[i];
        bufs[i]->mAudioDataByteSize = buffer_size;
        AQcallback(&p, queue, buf_refs[i]);
    }
    
    status = AudioQueueSetParameter (queue, kAudioQueueParam_Volume, 0.25f);
    
    status = AudioQueueStart (queue, NULL);
    
    while (p.cur_sample <= p.num_samples){
        CFRunLoopRunInMode (
                            kCFRunLoopDefaultMode,
                            time_per_loop, // seconds
                            false // don't return after source handled
                            );
    }
    
    CFRunLoopRunInMode ( kCFRunLoopDefaultMode,
                        time_per_loop,
                        false);
    
    AudioQueueDispose(queue, true);
    
    return 0;
}
