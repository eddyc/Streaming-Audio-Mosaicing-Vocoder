//
//  AnalysisData.h
//  ConcatenationRewrite
//
//  Created by Edward Costello on 23/01/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import <MacTypes.h>
#import <stdlib.h>
#import "Matrix.h"

/*!
 @group Data Structures
 */

/*!
 @class AudioAnalysisQueue
 @abstract Single-threaded work-loop client request mechanism.
 @discussion An IOCommandGate instance is an extremely light weight mechanism that
 executes an action on the driver's work-loop...
 @throws foo_exception
 @throws bar_exception
 @namespace I/O Kit (this is just a string)
 @updated 2003-03-15
 */



#ifdef __cplusplus
extern "C"
{
#endif
    
    typedef struct AudioAnalysisQueue32
    {
        size_t frameCount;
        size_t currentFrame;
        Matrix32 *mfccs;
        Matrix32 *chromagram;
        Matrix32 *magnitudes;
        Matrix32 *triangleFilteredMagnitudes;
        Matrix32 **triangleMagnitudeBands;
        Matrix32 *triangleFluxFilteredMagnitudes;
        Matrix32 **triangleFluxMagnitudeBands;
        size_t *triangleRowBlockSizes;
        size_t triangleMagnitudeBandsCount;
        
    } AudioAnalysisQueue32;
    
    AudioAnalysisQueue32 *AudioAnalysisQueue32_new(size_t FFTFrameSize,
                                                   size_t frameCount,
                                                   size_t triangleFilterSize,
                                                   size_t *triangleRowBlockSizes,
                                                   size_t triangleMagnitudeBandsCount);
    void AudioAnalysisQueue32_delete(AudioAnalysisQueue32 *self);
    void AudioAnalysisQueue32_toHDF(AudioAnalysisQueue32 *self, char *path);
    
    
#ifdef __cplusplus
}
#endif