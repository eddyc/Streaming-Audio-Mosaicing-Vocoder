//
//  AudioAnalysisData.h
//  ConcatenationRewrite
//
//  Created by Edward Costello on 23/01/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//





#import <MacTypes.h>
#import <stdlib.h>
#import "Matrix.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
    /*!
     @class AudioAnalysisData32
     @abstract A data structure that contains analysis data for a corresponding AudioObject
     @var sampleCount
     The number of audio samples in the analysed audio object.
     @var FFTFrameSize
     The FFT frame size at which audio analysis is was performed.
     @var hopSize
     The hop size at which audio analysis was performed
     @var hopCount
     The amount of analysis frames of the audio data.
     @var mfccs
     The matrix containing the MFCC analysis data.
     @var chromagram
     The matrix containing the chromagram analysis data.
     @var beats
     The matrix containing the hopCount position of the start of a beat in row 0, and the length of the beat in hopCounts in row 1.
     */
    typedef struct AudioAnalysisData32
    {
        size_t sampleCount;
        size_t samplerate;
        size_t FFTFrameSize;
        size_t hopSize;
        size_t hopCount;
        
        Matrix32 *mfccs;
        Matrix32 *chromagram;
        Matrix32 *beats;
        Matrix32 *magnitudes;
        Matrix32 *triangleMagnitudes;
        Matrix32 **triangleMagnitudeBands;
        Matrix32 *triangleFluxMagnitudes;
        Matrix32 **triangleFluxMagnitudeBands;
        size_t *triangleRowBlockSizes;
        size_t largestTriangleBandSize;
        size_t triangleMagnitudeBandsCount;
        Float32 *frameTimeInSeconds;
        size_t largestBeatSize;
        
    } AudioAnalysisData32;
    
    
    /*!
     @functiongroup Construction/Destruction
     */
    
    /*!
     Construct an AudioAnalysisData32 structure.
     @param sampleCount
     The sample count of an input audio file.
     @param FFTFrameSize
     The size of the FFT analysis frame being used with an AudioAnalysis pseudo-class, usually a power of 2 (e.g 1024);
     @param hopSize
     The hop size of the FFT analysis frames being used with an AudioAnalysis pseudo-class, when using a hanning window it should be at least FFTFrameSize / 4 or smaller.
     @result
     Returns an AudioAnalysisData32 pseudo-class structure. 
     */
    AudioAnalysisData32 *AudioAnalysisData32_new(size_t samplerate,
                                                 size_t sampleCount,
                                                 size_t FFTFrameSize,
                                                 size_t hopSize,
                                                 size_t triangleMagnitudesCount,
                                                 size_t triangleMagnitudeBandsCount);

    /*!
     This is a function.
     @param self
     Pointer to self.
     */
    void AudioAnalysisData32_delete(AudioAnalysisData32 *self);
    
    /*!
     @functiongroup Saving
     */
    /*!
     Save contents of AudioAnalysis32 structure to an hdf file.
     @param self
     Pointer to self.
     @param path
     Path to save hdf file, must contain .hdf extension.
     */
    void AudioAnalysisData32_saveToHDF(AudioAnalysisData32 *self, char *path);
    
    
    void AudioAnalysisData32_calculateTriangleFilterBandSizes(AudioAnalysisData32 *self);

    
#ifdef __cplusplus
}
#endif