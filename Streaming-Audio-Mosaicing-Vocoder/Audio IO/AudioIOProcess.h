//
//  AudioProcess.h
//  Audio-Mosaicing
//
//  Created by Edward Costello on 27/03/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//


#import <MacTypes.h>
#import <stdio.h>
#import <stdlib.h>
#import <AudioToolbox/AudioToolbox.h>
#import "AudioObject.h"
#import "CsoundObject.h"
#import "AudioAnalysisQueue.h"
#import "RingBuffer.h"
#import "AudioAnalyser.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
    typedef struct AudioIOProcess32
    {
        AudioUnit outputUnit;
        Float32 **stereoBuffer;
        Float32 *monoBuffer;
        Float32 *frameBuffer;
        size_t maximumSegmentFrameCount;
        size_t *bestTriangleBandMatches;
        Matrix32 *warpFrameTimesInSeconds;
        UInt32 channelCount;
        UInt32 samplesPerFrame;
        UInt32 framesPerCallback;
        UInt32 FFTFrameSize;
        UInt32 hopSize;
        AudioObject *audioObject;
        Boolean useBeats;
        Boolean useFlux;

        Float32 *segmentLengths;
        Float32 *segmentMagnitudeDifferences;

        RingBufferFloat32 *ringBuffer;
        AudioAnalysisQueue32 *analysisQueue;
        AudioAnalyser32 *audioAnalyser;
        AudioAnalysisData32 *paletteData;
        AudioObject *paletteAudioObject;
        CsoundObject *csoundObject;
        Matrix32 **analysisQueueComparisonData;
        size_t frameCount;
        
    } AudioIOProcess32;
    
    AudioIOProcess32 *AudioIOProcess32_new(AudioAnalyser32 *audioAnalyser,
                                           AudioObject *paletteAudioObject,
                                           AudioAnalysisData32 *paletteData,
                                           size_t maximumSegmentFrameCount,
                                           Boolean useBeats,
                                           Boolean useFlux);
    
    void AudioIOProcess32_delete(AudioIOProcess32 *self);
    void AudioIOProcess32_configureAudioUnit(AudioIOProcess32 *self);
    void AudioIOProcess32_configureCsound(AudioIOProcess32 *self);
    void AudioIOProcess32_process(AudioIOProcess32 *self,
                                  AudioObject *audioObject,
                                  UInt32 time);
    void AudioIOProcess32_processToAudioObject(AudioIOProcess32 *self,
                                               AudioObject *analysisAudioObject,
                                               AudioObject *saveFileAudioObject);
    
#ifdef __cplusplus
}
#endif