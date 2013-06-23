    //
    //  AudioAnalyser.h
    //  ConcatenationRewrite
    //
    //  Created by Edward Costello on 28/01/2013.
    //  Copyright (c) 2013 Edward Costello. All rights reserved.
    //

#import <MacTypes.h>
#import <stdlib.h>
#import "FFT.h"
#import "BeatDetect.h"
#import "DTW.h"
#import "TriangleFilterBank.h"
#import "AudioAnalysisData.h"
#import "AudioAnalysisQueue.h"
#import "AudioObject.h"
#import "OpenCLDTW.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
    /*!
     @class AudioAnalyser32
     @abstract A pseudoclass for performing analysis on AudioObjects and streaming frames of audio data
     @var FFTFrameSize
     The size of an audio analysis frame, this is usually a power of 2 (e.g 1024).
     @var FFTFrameSizeOver2
     Half the size of an FFT frame.
     @var hopSize
     The rate at which audio analysis frames are skipped, if using a hanning window the hop size must be at least FFTFrameSize/4 or smaller.
     @var fft
     A pointer to an <b>FFT32</b> pseudoclass
     @var mfcc
     A pointer to an <b>MFCC32</b> pseudoclass
     @var dtw
     A pointer to a <b>DTW32</b> pseudoclass, this pseudoclass is only allocated by an <b>AudioAnalysisProcess32</b> object
     @var dtwAllocated
     Flag for whether the <i>dtw</i> pseudoclass had been allocated
     @var chromagram
     A pointer to a Chromagram32 pseudoclass
     @var magnitudeBuffer
     A pointer to a Float32 buffer of FFTFrameSizeOver2 in length which is used to temporarily store spectral magnitudes during analysis.
     @var frameBuffer
     A pointer to a Float32 buffer of FFTFrameSize in length which is used to temporarily store audio samples during analysis.
     @var mfccBuffer
     A pointer to a Float32 buffer of <i>mfcc->ceptralCoefficients</i> in length which is used to temporarily store mel-frequency cepstral coefficients during analysis.
     @var chromagramBuffer
     A pointer to a Float32 buffer of <i>chromagram->nchr</i> in length which is used to temporarily store chromagrams during analysis.
     */
    
    typedef struct AudioAnalyser32
    {
        size_t FFTFrameSize;
        size_t FFTFrameSizeOver2;
        size_t hopSize;
        
        FFT32 *fft;
        DTW32 *magnitudesDTW;
        DTW32 *chromagramDTW;
        TriangleFilterBank32 *triangleFilterBank;
        size_t triangleMagnitudeBandsCount;
        Matrix32 *triangleBandGains;
        size_t *warpPath;
        Float32 *frameTimesInSeconds;
        size_t currentSegmentSize;
        Boolean dtwAllocated;
        OpenCLDTW **openclDTWs;
        Float32 *openclSimilarityScores;
        Float32 *frameBuffer;
        Float32 *mfccBuffer;
        Float32 *chromagramBuffer;
        Float32 *previousTriangleMagnitudes;
        Matrix32 **paletteComparisonData;

        
    } AudioAnalyser32;
    
    /*!
     @functiongroup Construct/Destruct
     */
    
    /*!
     @abstract Contruct an <b>AudioAnalyser32</b> pseudoclass
     @param samplerate
     Specify the <b>AudioObject</b> or streaming audio frames samplerate
     @param FFTFrameSize
     The size of an audio analysis frame, this is usually a power of 2 (e.g 1024).
     @param hopSize
     The rate at which audio analysis frames are skipped, if using a hanning window the hop size must be at least FFTFrameSize/4 or smaller.
     */
    
    AudioAnalyser32 *AudioAnalyser32_new(size_t samplerate,
                                         size_t FFTFrameSize,
                                         size_t hopSize,
                                         size_t triangleFilterCount,
                                         size_t triangleMagnitudeBandsCount);
    
    /*!
     @abstract Destroy an <b>AudioAnalyser32</b> pseudoclass
     @var self
     A pointer to an <b>AudioAnalyser32</b> pseudoclass
     */
    
    void AudioAnalyser32_delete(AudioAnalyser32 *self);
    
    void AudioAnalyser32_allocateDTW(AudioAnalyser32 *self,
                                     AudioAnalysisData32 *paletteAnalysisData,
                                     size_t rowCount,
                                     size_t columnCount,
                                     Boolean useBeats,
                                     Boolean useFlux);
    
    
    /*!
     @functiongroup Audio analysis
     */
    
    /*!
     @abstract Analyse an <b>AudioObject</b>, deriving high level feature vectors.
     @param self
     A pointer to an <b>AudioAnalyser32</b> pseudoclass
     @param audioObject
     A pointer to the AudioObject to be analysed.
     @param analysisData
     A pointer to an <b>AudioAnalysisData32</b> pseudoclass which stores the analysed feature vectors.
     @discussion
     This pseudoclass performs chromagram, mel-frequency cepstral coefficient and beat detection analysis on an AudioObject.
     */
    
    
    
    void AudioAnalyser32_analyseAudioObject(AudioAnalyser32 *self,
                                            AudioObject *audioObject,
                                            AudioAnalysisData32 *paletteData,
                                            Float32 tempoMean);
    
    /*!
     @abstract Analyse a frame of audio, deriving high level feature vectors.
     @param self
     A pointer to an <b>AudioAnalyser32</b> pseudoclass
     @param audioFrame
     A pointer to an array of audio samples
     @param analysisQueue
     A pointer to an <b>AudioAnalysisQueue32</b> pseudoclass which stores a specified frame count of the analysed feature vectors.
     @discussion
     This pseudoclass performs chromagram, mel-frequency cepstral coefficient and beat detection analysis on an incoming stream of audio frames. After each frames analysis the <i>currentFrame</i> variable of the <b>AudioAnalysisQueue32</b> pseudoclass is incremented wrapping around to zero when it is greater than the queues size.
     */
    
    void AudioAnalyser32_analyseAudioFrameToQueue(AudioAnalyser32 *self,
                                                  Float32 *audioFrame,
                                                  AudioAnalysisQueue32 *analysisQueue,
                                                  AudioAnalysisData32 *paletteData);
    
    void AudioAnalyser32_findTriangleFilterBandGains(AudioAnalyser32 *self,
                                                     AudioAnalysisData32 *paletteData);
    
    /*!
     @abstract Compare analysis data in an <b>AudioAnalysisQueue32</b> pseudoclass with data in an <b>AudioAnalysisData32</b> pseudoclass using dynamic time warping.
     @param analysisQueue
     A pointer to an <b>AudioAnalysisQueue32</b> pseudoclass
     @param analysisData
     A pointer to an <b>AudioAnalysisData32</b> pseudoclass
     @discussion
     This pseudoclass compares the high level features in the <b>AudioAnalysisQueue32</b> pseudoclass with those in an <b>AudioAnalysisData32</b> pseudoclass using dynamic time warping.
     */
    size_t AudioAnalyser32_findBestMatch(AudioAnalyser32 *self,
                                         Matrix32 *analysisData,
                                         Matrix32 *paletteData,
                                         Float32 *warpFrameTimesInSeconds);
    
    void AudioAnalyser32_findBestTriangleBandMatches(AudioAnalyser32 *self,
                                                     AudioAnalysisQueue32 *analysisQueue,
                                                     AudioAnalysisData32 *paletteData,
                                                     size_t *bestMatches,
                                                     Matrix32 *warpFrameTimesInSeconds);
    
    void AudioAnalyser32_splitTriangleFilterMagnitudesIntoBands(AudioAnalyser32 *self,
                                                                AudioAnalysisData32 *paletteData,
                                                                Float32 *triangleMagnitudes,
                                                                Matrix32 **triangleMagnitudeBands,
                                                                size_t rowIndex);
    
    void AudioAnalyser32_findMatchOpenCL(AudioAnalyser32 *self,
                                         Matrix32 **triangleMagnitudeBands,
                                         Float32 *warpFrameTimesInSeconds,
                                         size_t *bestTriangleBandMatches);
    
    void AudioAnalyser32_findMagnitudeDifferences(AudioAnalyser32 *self,
                                                  Matrix32 **analysisTriangleMagnitudeBands,
                                                  Matrix32 **paletteTriangleMagnitudeBands,
                                                  Float32 *segmentLengths,
                                                  size_t *bestTriangleBandMatches,
                                                  Float32 *magnitudeDifferences);
#ifdef __cplusplus
}
#endif