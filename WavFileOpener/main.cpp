//
//  main.cpp
//  WavFileOpener
//
//  Created by John Asper on 2016/8/10.
//  Copyright © 2016 John Asper. All rights reserved.
//

#include <iostream>
#include <AudioToolbox/AudioToolbox.h>

#include "WavFile.hpp"

typedef struct {
    WavFile *w;
    uint32_t cur_sample;
    uint32_t num_samples;
    AudioStreamBasicDescription fmt;
} Player;

void AQcallback (void *ptr, AudioQueueRef queue, AudioQueueBufferRef buf_ref){
    Player *p = (Player *)ptr;
    OSStatus status;
    AudioQueueBuffer *buf = buf_ref;
    uint32_t nsamples = buf->mAudioDataByteSize / p->fmt.mBytesPerPacket;
    uint16_t *samp = (uint16_t *)buf->mAudioData;
    
    printf("Callback! nsampl: %d\n", nsamples);
    
    int sample = 0;
    while(sample < nsamples){
        for (int channel = 0; channel < p->w->getNumChannels(); ++channel) {
            if (p->cur_sample > p->num_samples) {
                samp[sample] = 0;
            }
            samp[sample] = p->w->getData()[channel][p->cur_sample];
            ++sample;
        }
        ++(p->cur_sample);
    }
    status = AudioQueueEnqueueBuffer (queue, buf_ref, 0, NULL);
    printf ("Enqueue status: %d\n", status);
}

static const int MAXPATHLEN = 65535;
std::string pwd(){
    char temp[MAXPATHLEN];
    return ( getcwd(temp, MAXPATHLEN)) ? std::string(temp) : std::string("");
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

int main(int argc, const char * argv[]) {
    std::cout << "Current directory is: " << pwd() << "\n";
    WavFile wav = WavFile("/Users/john/Documents/Xcode Projects/WavFileOpener/test.wav");
    Player p = {&wav, 0, wav.getNumSamples(), {0}};
    
    AudioQueueRef queue;
    OSStatus status;
    AudioQueueBufferRef buf_ref1, buf_ref2;
    
    p.fmt.mSampleRate = wav.getSampleRate();
    p.fmt.mFormatID = kAudioFormatLinearPCM;
    p.fmt.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    p.fmt.mFramesPerPacket = 1;
    p.fmt.mChannelsPerFrame = wav.getNumChannels();
    p.fmt.mBytesPerPacket = p.fmt.mBytesPerFrame = sizeof(uint16_t)*wav.getNumChannels();
    p.fmt.mBitsPerChannel = sizeof(uint16_t)*8;
    
    status = AudioQueueNewOutput(&(p.fmt), AQcallback, &p, CFRunLoopGetCurrent(),
                                 kCFRunLoopCommonModes, 0, &queue);
    
    std::cout << "New Output Status: " << getError(status) << std::endl;
    
    if (status == kAudioFormatUnsupportedDataFormatError) puts ("oops!");
    else printf("NewOutput status: %d\n", status);
    
    status = AudioQueueAllocateBuffer(queue, 20000, &buf_ref1);
    printf("Allocate Status: %d\n", status);
    
    AudioQueueBuffer *buf = buf_ref1;
    printf ("buf: %p, data: %p, len: %d\n", buf, buf->mAudioData, buf->mAudioDataByteSize);
    buf->mAudioDataByteSize = 20000;
    
    AQcallback(&p, queue, buf_ref1);
    
    status = AudioQueueAllocateBuffer (queue, 20000, &buf_ref2);
    printf ("Allocate2 status: %d\n", status);
    
    buf = buf_ref2;
    buf->mAudioDataByteSize = 20000;
    
    AQcallback(&p, queue, buf_ref2);
    
    status = AudioQueueSetParameter (queue, kAudioQueueParam_Volume, 1.0);
    printf ("Volume status: %d\n", status);
    
    status = AudioQueueStart (queue, NULL);
    printf ("Start status: %d\n", status);
    
    while (p.cur_sample <= p.num_samples){
        CFRunLoopRunInMode (
                            kCFRunLoopDefaultMode,
                            0.25, // seconds
                            false // don't return after source handled
                            );
    }
    
    CFRunLoopRunInMode ( kCFRunLoopDefaultMode,
                        1,
                        false);
    
    AudioQueueDispose(queue, true);
    
    return 0;
}
