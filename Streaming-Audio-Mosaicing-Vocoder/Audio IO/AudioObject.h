//
//  AudioObject.h
//  Audio IO Tester
//
//  Created by Edward Costello on 29/11/2012.
//  Copyright (c) 2012 Edward Costello. All rights reserved.
//

#import <MacTypes.h>
#import <stdlib.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Accelerate/Accelerate.h>


#ifdef __cplusplus
extern "C"
{
#endif
    
    typedef enum CanonicalAudioStreamBasicDescription
    {
        kProcessStreamDescription,
        kFileStreamDescription
        
    } CanonicalAudioStreamBasicDescription;
        
    typedef struct AudioObject
    {
        Float32 **audioChannels;
        size_t audioChannelCapacity;
        
        Float32 *channelMono;
        size_t *writeIndex;
        
        UInt16 channelCount;
        size_t frameCount;
        size_t currentFramePosition;
        
        UInt32 samplerate;
        AudioStreamBasicDescription *fileStreamDescription;
        AudioStreamBasicDescription *processStreamDescription;
                
        char *filePath;
        Boolean populated;
        
    } AudioObject;
    
    AudioObject *AudioObject_new();
    AudioObject *AudioObject_newBlank(UInt8 channelCount, size_t frameCount);

    
    void AudioObject_delete(AudioObject *);
    
    void AudioObject_openAudioFile(AudioObject *,
                                   const char *path);
    
    void AudioObject_saveAudioFile(AudioObject *,
                                   const char *path);
    
    void AudioObject_setupCanonicalAudioStreamBasicDescription(AudioObject *,
                                                               CanonicalAudioStreamBasicDescription description);
    

    void AudioObject_readSamples(AudioObject *,
                                 SInt32 readOffset,
                                 SInt32 channel,
                                 Float32 *__restrict output,
                                 size_t outputSize);
    

    void AudioObject_writeBufferOfSamples(AudioObject *,
                                          size_t channel,
                                          Float32 *buffer,
                                          size_t bufferSize);
    
    void AudioObject_writeBuffersOfSamples(AudioObject *,
                                           size_t channelCount,
                                           Float32 **buffers,
                                           size_t bufferSize);
    
    Float32 *AudioObject_reallocateArray(Float32 *array,
                                         size_t oldSize,
                                         size_t newSize);
    
    void AudioObject_readCallbackStereo(void *inRefCon, UInt32 inNumberFrames, Float32 **audioData);
    void AudioObject_readCallbackMono(void *inRefCon, UInt32 inNumberFrames, Float32 *audioData);

#ifdef __cplusplus
}
#endif