    //
    //  AudioAnalyser.c
    //  ConcatenationRewrite
    //
    //  Created by Edward Costello on 28/01/2013.
    //  Copyright (c) 2013 Edward Costello. All rights reserved.
    //

#import "AudioAnalyser.h"
#import "MathematicalFunctions.h"
#import "ConvenienceFunctions.h"

#pragma mark AudioAnalyser32

AudioAnalyser32 *AudioAnalyser32_new(size_t samplerate,
                                     size_t FFTFrameSize,
                                     size_t hopSize,
                                     size_t triangleFilterCount,
                                     size_t triangleMagnitudeBandsCount)
{
    AudioAnalyser32 *self = calloc(1, sizeof(AudioAnalyser32));
    self->FFTFrameSize = nextPowerOfTwo(FFTFrameSize);
    self->FFTFrameSizeOver2 = self->FFTFrameSize / 2;
    self->hopSize = hopSize;
    self->triangleMagnitudeBandsCount = triangleMagnitudeBandsCount;
    self->fft = FFT32_new(self->FFTFrameSize, kFFTWindowType_Hanning);

    self->triangleFilterBank = TriangleFilterBank32_new(triangleFilterCount, self->FFTFrameSizeOver2, samplerate);
    self->triangleBandGains = Matrix32_new(self->triangleMagnitudeBandsCount, self->FFTFrameSizeOver2);
    self->dtwAllocated = false;
    self->frameBuffer = calloc(self->FFTFrameSize, sizeof(Float32));
    self->previousTriangleMagnitudes = calloc(triangleFilterCount, sizeof(Float32));
    return self;
}

void AudioAnalyser32_allocateDTW(AudioAnalyser32 *self,
                                 AudioAnalysisData32 *paletteAnalysisData,
                                 size_t rowCount,
                                 size_t columnCount,
                                 Boolean useBeats,
                                 Boolean useFlux)
{
    self->magnitudesDTW = DTW32_new(rowCount, columnCount);
    
    self->openclDTWs = calloc(self->triangleMagnitudeBandsCount, sizeof(OpenCLDTW *));
    
    
    if (useFlux == true) {
        
        self->paletteComparisonData  = paletteAnalysisData->triangleFluxMagnitudeBands;
    }
    else {
        
        self->paletteComparisonData = paletteAnalysisData->triangleMagnitudeBands;
    }
    
    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        size_t currentColumnCount = paletteAnalysisData->triangleRowBlockSizes[i];
        
        self->openclDTWs[i] = OpenCLDTW_new(OpenCLDTW_useCPU,
                                            rowCount,
                                            currentColumnCount,
                                            self->paletteComparisonData[i]->data,
                                            self->paletteComparisonData[i]->rowCount,
                                            paletteAnalysisData->beats,
                                            useBeats);
        
    }
    
    self->openclSimilarityScores = calloc(self->openclDTWs[0]->globalWorkSize, sizeof(Float32));
    
    self->warpPath = calloc(rowCount, sizeof(size_t));
    
    self->frameTimesInSeconds = calloc(rowCount, sizeof(size_t));
    
    for (size_t i = 0; i < rowCount; ++i) {
        
        self->frameTimesInSeconds[i] = (Float32)i * (1. / (Float32)44100) * (Float32)self->hopSize;
    }
    
    self->dtwAllocated = true;
}

void AudioAnalyser32_delete(AudioAnalyser32 *self)
{
    FFT32_delete(self->fft);
    TriangleFilterBank32_delete(self->triangleFilterBank);
    Matrix32_delete(self->triangleBandGains);
    if (self->dtwAllocated == true) {
        
        DTW32_delete(self->magnitudesDTW);
        
        for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
            
            OpenCLDTW_delete(self->openclDTWs[i]);
        }
        
        free(self->openclDTWs);
        free(self->openclSimilarityScores);
        free(self->frameTimesInSeconds);
        free(self->warpPath);
    }
    
    free(self->frameBuffer);
    free(self->mfccBuffer);
    free(self->chromagramBuffer);
    free(self->previousTriangleMagnitudes);
    free(self);
    self = NULL;
}

static void AudioAnalyser32_analyseBeats(AudioAnalyser32 *self,
                                         AudioObject *audioObject,
                                         AudioAnalysisData32 *analysisData,
                                         Float32 tempoMean)
{
    BeatDetect32 *beatDetect = BeatDetect32_new(audioObject->samplerate, audioObject->frameCount, tempoMean);
    
    BeatDetect32_process(beatDetect, audioObject->channelMono, analysisData->beats->data, &analysisData->beats->columnCount);
    analysisData->beats->elementCount = analysisData->beats->columnCount * 2;
    
    Float32 *beatPositions = Matrix_getRow(analysisData->beats, 0);
    Float32 *beatLengths = Matrix_getRow(analysisData->beats, 1);
    size_t beatsCount = analysisData->beats->columnCount;
    
    for (size_t i = 0; i < analysisData->beats->columnCount - 1; ++i) {
        
        beatLengths[i] = beatPositions[i + 1] - beatPositions[i];
    }
    beatLengths[beatsCount - 1] = analysisData->hopCount - beatPositions[beatsCount - 1];
    
    Float32 longestBeat;
    vDSP_maxv(beatLengths, 1, &longestBeat, analysisData->beats->columnCount);
    
    analysisData->largestBeatSize = (size_t)longestBeat;
    
    
        //
        //    if (maximumSongBeatSize < maximumSegmentCount) {
        //
        //        BeatDetect32_delete(beatDetect);
        //        return;
        //    }
        //
        //    Float32 *beatsWithEnd = calloc(beatsCount + 1, sizeof(Float32));
        //    cblas_scopy((SInt32)beatsCount, beatPositions, 1, beatsWithEnd, 1);
        //    beatsWithEnd[beatsCount] = analysisData->hopCount;
        //
        //    while (maximumSongBeatSize > maximumSegmentCount) {
        //
        //        maximumSongBeatSize = roundf(maximumSongBeatSize / 2);
        //        subDiviser++;
        //    }
        //
        //    subDiviser = powf(2, subDiviser);
        //    UInt32 newBeatsCount = beatsCount * subDiviser;
        //
        //    analysisData->beats->elementCount = newBeatsCount * 2;
        //    analysisData->beats->columnCount = newBeatsCount;
        //    beatPositions = Matrix_getRow(analysisData->beats, 0);
        //    beatLengths = Matrix_getRow(analysisData->beats, 1);
        //
        //    Float32 *indexes = calloc(newBeatsCount, sizeof(Float32));
        //    Float32 *newBeats = calloc(newBeatsCount, sizeof(Float32));
        //    SInt32 *newBeatsAsInt = calloc(newBeatsCount, sizeof(SInt32));
        //
        //    Float32 zero = 0;
        //    Float32 increment = 1./subDiviser;
        //    vDSP_vramp(&zero, &increment, indexes, 1, newBeatsCount);
        //
        //    vDSP_vlint(beatsWithEnd, indexes, 1, newBeats, 1, newBeatsCount, beatsCount + 1);
        //    vDSP_vfixr32(newBeats, 1, newBeatsAsInt, 1, newBeatsCount);
        //    vDSP_vflt32(newBeatsAsInt, 1, newBeats, 1, newBeatsCount);
        //
        //    cblas_scopy(newBeatsCount, newBeats, 1, beatPositions, 1);
        //
        //
        //    for (size_t i = 0; i < newBeatsCount - 1; ++i) {
        //
        //        beatLengths[i] = beatPositions[i + 1] - beatPositions[i];
        //    }
        //
        //    beatLengths[newBeatsCount - 1] = analysisData->hopCount - beatPositions[newBeatsCount - 1];
        //
        //    free(newBeats);
        //    free(newBeatsAsInt);
        //    free(beatsWithEnd);
        //    free(indexes);
    
    BeatDetect32_delete(beatDetect);
    
}

void AudioAnalyser32_analyseAudioObject(AudioAnalyser32 *self,
                                        AudioObject *audioObject,
                                        AudioAnalysisData32 *paletteData,
                                        Float32 tempoMean)
{
    
    for (SInt64 i = 0; i < paletteData->hopCount; ++i) {
        
        AudioObject_readSamples(audioObject, (SInt32)(i * self->hopSize), -1, self->frameBuffer, self->FFTFrameSize);

        FFT32_forwardMagnitudesWindowed(self->fft, self->frameBuffer, Matrix_getRow(paletteData->magnitudes, i));
    
        TriangleFilterBank32_process(self->triangleFilterBank, Matrix_getRow(paletteData->magnitudes, i), Matrix_getRow(paletteData->triangleMagnitudes, i));
        
        vDSP_vsub(Matrix_getRow(paletteData->triangleMagnitudes, i),
                  1,
                  self->previousTriangleMagnitudes,
                  1,
                  Matrix_getRow(paletteData->triangleFluxMagnitudes, i),
                  1,
                  paletteData->triangleFluxMagnitudes->columnCount);
        
        vDSP_vabs(Matrix_getRow(paletteData->triangleFluxMagnitudes, i), 1, Matrix_getRow(paletteData->triangleFluxMagnitudes, i), 1, paletteData->triangleFluxMagnitudes->columnCount);
        
        cblas_scopy((UInt32)paletteData->triangleFluxMagnitudes->columnCount, Matrix_getRow(paletteData->triangleMagnitudes, i), 1, self->previousTriangleMagnitudes, 1);
        
        AudioAnalyser32_splitTriangleFilterMagnitudesIntoBands(self, paletteData, Matrix_getRow(paletteData->triangleMagnitudes, i), paletteData->triangleMagnitudeBands, i);
        
        AudioAnalyser32_splitTriangleFilterMagnitudesIntoBands(self, paletteData, Matrix_getRow(paletteData->triangleFluxMagnitudes, i), paletteData->triangleFluxMagnitudeBands, i);
        
    }
    
    vDSP_vclr(self->previousTriangleMagnitudes, 1, self->triangleFilterBank->filterCount);
    AudioAnalyser32_analyseBeats(self, audioObject, paletteData, tempoMean);
    AudioAnalyser32_findTriangleFilterBandGains(self, paletteData);
}

void AudioAnalyser32_analyseAudioFrameToQueue(AudioAnalyser32 *self,
                                              Float32 *audioFrame,
                                              AudioAnalysisQueue32 *analysisQueue,
                                              AudioAnalysisData32 *paletteData)
{
    size_t currentQueueFrame = analysisQueue->currentFrame;
    Float32 *currentMagnitudes = Matrix_getRow(analysisQueue->magnitudes, currentQueueFrame);
    Float32 *currentTriangleFilteredMagnitudes = Matrix_getRow(analysisQueue->triangleFilteredMagnitudes, currentQueueFrame);
    Float32 *currentTriangleFluxFilteredMagnitudes = Matrix_getRow(analysisQueue->triangleFluxFilteredMagnitudes, currentQueueFrame);
    
    FFT32_forwardMagnitudesWindowed(self->fft, audioFrame, currentMagnitudes);
    TriangleFilterBank32_process(self->triangleFilterBank, currentMagnitudes, currentTriangleFilteredMagnitudes);
    
    vDSP_vsub(currentTriangleFilteredMagnitudes,
              1,
              self->previousTriangleMagnitudes,
              1,
              currentTriangleFluxFilteredMagnitudes,
              1,
              self->triangleFilterBank->filterCount);
    
    vDSP_vabs(currentTriangleFluxFilteredMagnitudes, 1, currentTriangleFluxFilteredMagnitudes, 1, self->triangleFilterBank->filterCount);
    
    cblas_scopy((UInt32)self->triangleFilterBank->filterCount, currentTriangleFilteredMagnitudes, 1, self->previousTriangleMagnitudes, 1);
    
    
    AudioAnalyser32_splitTriangleFilterMagnitudesIntoBands(self, paletteData, currentTriangleFilteredMagnitudes, analysisQueue->triangleMagnitudeBands, currentQueueFrame);
    AudioAnalyser32_splitTriangleFilterMagnitudesIntoBands(self, paletteData, currentTriangleFluxFilteredMagnitudes, analysisQueue->triangleFluxMagnitudeBands, currentQueueFrame);
    
    analysisQueue->currentFrame = (analysisQueue->currentFrame + 1) % analysisQueue->frameCount;
}

void AudioAnalyser32_splitTriangleFilterMagnitudesIntoBands(AudioAnalyser32 *self,
                                                            AudioAnalysisData32 *paletteData,
                                                            Float32 *triangleMagnitudes,
                                                            Matrix32 **triangleMagnitudeBands,
                                                            size_t rowIndex)
{
    size_t currentIndex = 0;
    for (size_t i = 0; i < paletteData->triangleMagnitudeBandsCount; ++i) {
        
        size_t currentBandBlockSize = paletteData->triangleRowBlockSizes[i];
        Matrix32 *currentBandMatrix = triangleMagnitudeBands[i];
        cblas_scopy((SInt32)currentBandBlockSize, &triangleMagnitudes[currentIndex], 1, Matrix_getRow(currentBandMatrix, rowIndex), 1);
        
        currentIndex += currentBandBlockSize;
    }
    
}

void AudioAnalyser32_findTriangleFilterBandGains(AudioAnalyser32 *self,
                                                 AudioAnalysisData32 *paletteData)
{
    
    size_t currentIndex = 0;
    
    for (size_t i = 0; i < paletteData->triangleMagnitudeBandsCount; ++i) {
        
        size_t currentRowBlockSize = paletteData->triangleRowBlockSizes[i];
        
        for (size_t j = 0; j < currentRowBlockSize; ++j) {
            
            Float32 *currentFilter = &self->triangleFilterBank->filterBank[currentIndex + j];
            
            vDSP_vadd(currentFilter, (SInt32)self->triangleFilterBank->filterCount, Matrix_getRow(self->triangleBandGains, i), 1, Matrix_getRow(self->triangleBandGains, i), 1, self->FFTFrameSizeOver2);
        }
        
        Float32 zero = 0, one = 1;
        vDSP_vclip(Matrix_getRow(self->triangleBandGains, i), 1, &zero, &one, Matrix_getRow(self->triangleBandGains, i), 1, self->FFTFrameSizeOver2);
        
        currentIndex += currentRowBlockSize;
    }
    
    for (size_t i = 0 ; i < self->FFTFrameSizeOver2 - 1; ++i) {
        
        if (Matrix_getRow(self->triangleBandGains, 0)[i] < Matrix_getRow(self->triangleBandGains, 0)[i + 1]) {
            
            Matrix_getRow(self->triangleBandGains, 0)[i] = 1.;
        }
        else {
            
            break;
        }
    }
    
    for (size_t i = self->FFTFrameSizeOver2 - 1; i > 0; --i) {
        
        if (Matrix_getRow(self->triangleBandGains, (paletteData->triangleMagnitudeBandsCount - 1))[i] <= Matrix_getRow(self->triangleBandGains, (paletteData->triangleMagnitudeBandsCount - 1))[i - 1]) {
            
            Matrix_getRow(self->triangleBandGains, (paletteData->triangleMagnitudeBandsCount - 1))[i] = 1.;
        }
        else {
            
            break;
        }
        
    }
}

size_t AudioAnalyser32_findBestMatch(AudioAnalyser32 *self,
                                     Matrix32 *analysisData,
                                     Matrix32 *paletteData,
                                     Float32 *warpFrameTimesInSeconds)
{
    size_t bestMatch = 0;
    Float32 bestScore = INFINITY;
    
    for (size_t i = 0; i < paletteData->rowCount - analysisData->rowCount; i += analysisData->rowCount) {
        
        Float32 currentScore = DTW32_getSimilarityScore(self->magnitudesDTW,
                                                        analysisData->data,
                                                        analysisData->rowCount,
                                                        Matrix_getRow(paletteData, i),
                                                        analysisData->rowCount,
                                                        analysisData->columnCount);
        if (currentScore < bestScore) {
            
            DTW32_traceWarpPath(self->magnitudesDTW, self->warpPath);
            bestScore = currentScore;
            bestMatch = i;
        }
    }
    
    vDSP_vgathr(self->frameTimesInSeconds, self->warpPath, 1, warpFrameTimesInSeconds, 1, analysisData->rowCount);
    
    return bestMatch;
}

void AudioAnalyser32_findBestTriangleBandMatches(AudioAnalyser32 *self,
                                                 AudioAnalysisQueue32 *analysisQueue,
                                                 AudioAnalysisData32 *paletteData,
                                                 size_t *bestMatches,
                                                 Matrix32 *warpFrameTimesInSeconds)
{
    
    for (size_t currentBand = 0; currentBand < paletteData->triangleMagnitudeBandsCount; ++currentBand) {
        
        bestMatches[currentBand] = AudioAnalyser32_findBestMatch(self,
                                                                 analysisQueue->triangleMagnitudeBands[currentBand],
                                                                 paletteData->triangleMagnitudeBands[currentBand],
                                                                 Matrix_getRow(warpFrameTimesInSeconds, currentBand));
    }
}

void AudioAnalyser32_findMatchOpenCL(AudioAnalyser32 *self,
                                     Matrix32 **triangleMagnitudeBands,
                                     Float32 *warpFrameTimesInSeconds,
                                     size_t *bestTriangleBandMatches)
{
    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        OpenCLDTW_process(self->openclDTWs[i], triangleMagnitudeBands[i]->data, self->openclSimilarityScores);
        Float32 minimum = 0;
        size_t index = 0;
        vDSP_minvi(self->openclSimilarityScores, 1, &minimum, &index, self->openclDTWs[0]->globalWorkSize);
        
        bestTriangleBandMatches[i] = index;
    }
    
    printf("match index = %zd\n", bestTriangleBandMatches[0]);
}

void AudioAnalyser32_findMagnitudeDifferences(AudioAnalyser32 *self,
                                              Matrix32 **analysisTriangleMagnitudeBands,
                                              Matrix32 **paletteTriangleMagnitudeBands,
                                              Float32 *segmentLengths,
                                              size_t *bestTriangleBandMatches,
                                              Float32 *magnitudeDifferences)
{
    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        size_t currentPaletteSegmentLength = (size_t)segmentLengths[i];
        size_t currentPaletteSegmentIndex = bestTriangleBandMatches[i];
        Matrix32 *currentPaletteBandSegment = paletteTriangleMagnitudeBands[i];
        Matrix32 *currentAnalysisBandSegment = analysisTriangleMagnitudeBands[i];
        
        Float32 analysisSum;
        vDSP_sve(currentAnalysisBandSegment->data, 1, &analysisSum, currentAnalysisBandSegment->elementCount);
        Float32 paletteSum;
        vDSP_sve(Matrix_getRow(currentPaletteBandSegment, currentPaletteSegmentIndex), 1, &paletteSum, currentPaletteSegmentLength * currentPaletteBandSegment->columnCount);
        magnitudeDifferences[i] = analysisSum / paletteSum;

    }
}
