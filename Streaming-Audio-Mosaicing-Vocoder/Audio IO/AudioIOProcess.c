    //
    //  AudioProcess.c
    //  Audio-Mosaicing
    //
    //  Created by Edward Costello on 27/03/2013.
    //  Copyright (c) 2013 Edward Costello. All rights reserved.
    //

#import "AudioIOProcess.h"
#import "ConvenienceFunctions.h"
#import <Accelerate/Accelerate.h>

static const UInt32 warpPathsBaseTableNumber = 200;
static const UInt32 bandGainsBaseTableNumber = 300;

AudioIOProcess32 *AudioIOProcess32_new(AudioAnalyser32 *audioAnalyser,
                                       AudioObject *paletteAudioObject,
                                       AudioAnalysisData32 *paletteData,
                                       size_t maximumSegmentFrameCount,
                                       Boolean useBeats,
                                       Boolean useFlux)
{
    AudioIOProcess32 *self = calloc(1, sizeof(AudioIOProcess32));
    
    self->paletteAudioObject = paletteAudioObject;
    self->paletteData = paletteData;
    self->maximumSegmentFrameCount = maximumSegmentFrameCount;
    self->audioAnalyser = audioAnalyser;
    self->useBeats = useBeats;
    self->useFlux = useFlux;
    self->samplesPerFrame = 256;
    self->framesPerCallback = 2;
    self->channelCount = 2;
    self->FFTFrameSize = 1024;
    self->hopSize = 256;
    self->frameBuffer = calloc(self->FFTFrameSize, sizeof(Float32));
    self->stereoBuffer = calloc(self->channelCount, sizeof(Float32 *));
    self->bestTriangleBandMatches = calloc(paletteData->triangleMagnitudeBandsCount, sizeof(size_t));
    self->segmentMagnitudeDifferences = calloc(paletteData->triangleMagnitudeBandsCount, sizeof(Float32));
    
    self->warpFrameTimesInSeconds = Matrix32_new(paletteData->triangleMagnitudeBandsCount, self->maximumSegmentFrameCount);
    self->segmentLengths = calloc(paletteData->triangleMagnitudeBandsCount, sizeof(size_t));
    Float32 filler = (Float32)maximumSegmentFrameCount;
    vDSP_vfill(&filler, self->segmentLengths, 1, paletteData->triangleMagnitudeBandsCount);
    
    for (size_t i = 0; i < self->channelCount; ++i) {
        
        self->stereoBuffer[i] = calloc(self->samplesPerFrame, sizeof(Float32));
    }
    
    self->monoBuffer = calloc(self->samplesPerFrame, sizeof(Float32));
    
    self->ringBuffer = RingBufferFloat32_new(self->FFTFrameSize);
    
    self->analysisQueue = AudioAnalysisQueue32_new(self->FFTFrameSize,
                                                   maximumSegmentFrameCount,
                                                   audioAnalyser->triangleFilterBank->filterCount,
                                                   paletteData->triangleRowBlockSizes,
                                                   paletteData->triangleMagnitudeBandsCount);
    AudioIOProcess32_configureAudioUnit(self);
    
    if (self->useFlux == true) {
        
        self->analysisQueueComparisonData = self->analysisQueue->triangleFluxMagnitudeBands;
    }
    else {
        
        self->analysisQueueComparisonData = self->analysisQueue->triangleMagnitudeBands;
    }
    
    AudioAnalyser32_allocateDTW(self->audioAnalyser,
                                self->paletteData,
                                self->maximumSegmentFrameCount,
                                self->audioAnalyser->FFTFrameSizeOver2,
                                useBeats,
                                useFlux);
    
    AudioIOProcess32_configureCsound(self);
    
    return self;
}

void AudioIOProcess32_delete(AudioIOProcess32 *self)
{
    RingBufferFloat32_delete(self->ringBuffer);
    AudioAnalysisQueue32_delete(self->analysisQueue);
    Matrix32_delete(self->warpFrameTimesInSeconds);
    
    for (size_t i = 0; i < self->channelCount; ++i) {
        
        free(self->stereoBuffer[i]);
    }
    
    free(self->stereoBuffer);
    free(self->monoBuffer);
    free(self->frameBuffer);
    free(self->bestTriangleBandMatches);
    free(self->segmentLengths);
    free(self->segmentMagnitudeDifferences);
    free(self);
    self = NULL;
}

static void CheckError(OSStatus error, const char *operation)
{
	if (error == noErr) return;
	
	char str[20];
        // see if it appears to be a 4-char-code
	*(UInt32 *)(str + 1) = CFSwapInt32HostToBig(error);
	if (isprint(str[1]) && isprint(str[2]) && isprint(str[3]) && isprint(str[4])) {
		str[0] = str[5] = '\'';
		str[6] = '\0';
	} else
            // no, format it as an integer
		sprintf(str, "%d", (int)error);
	
	fprintf(stderr, "Error: %s (%s)\n", operation, str);
	
	exit(1);
}

static void AudioIOProcess32_writeAudioToOutput(AudioIOProcess32 *self,
                                                Float32 **audioData,
                                                AudioBufferList *ioData,
                                                size_t slice)
{
    
    for (size_t channel = 0; channel < ioData->mNumberBuffers; ++channel) {
        
        Float32 *currentBuffer = ioData->mBuffers[channel].mData;
        
        cblas_scopy((SInt32)self->samplesPerFrame,
                    self->stereoBuffer[channel],
                    1,
                    &currentBuffer[slice * self->samplesPerFrame],
                    1);
        
    }
}

static OSStatus AudioIOProcess32_RenderProc(void *inRefCon,
                                            AudioUnitRenderActionFlags *ioActionFlags,
                                            const AudioTimeStamp *inTimeStamp,
                                            UInt32 inBusNumber,
                                            UInt32 inNumberFrames,
                                            AudioBufferList *ioData)
{
    
    AudioIOProcess32 *self = (AudioIOProcess32 *)inRefCon;
    
    for (size_t currentFrame = 0; currentFrame < self->framesPerCallback; ++currentFrame, self->frameCount++) {
        
        AudioObject_readCallbackMono(self->audioObject,
                                     self->samplesPerFrame,
                                     self->monoBuffer);
        
        RingBufferFloat32_writeCopySkip(self->ringBuffer,
                                        self->monoBuffer,
                                        self->hopSize,
                                        self->frameBuffer,
                                        self->FFTFrameSize,
                                        self->hopSize);
        
        if (self->frameCount >= 3) {
            
            AudioAnalyser32_analyseAudioFrameToQueue(self->audioAnalyser,
                                                     self->frameBuffer,
                                                     self->analysisQueue,
                                                     self->paletteData);
            
            if (self->analysisQueue->currentFrame == 0) {
                
                
                AudioAnalyser32_findMatchOpenCL(self->audioAnalyser,
                                                self->analysisQueueComparisonData,
                                                self->warpFrameTimesInSeconds->data,
                                                self->bestTriangleBandMatches);
                
                if (self->useBeats) {
                    
                    for (size_t i = 0; i < self->paletteData->triangleMagnitudeBandsCount; ++i) {
                        
                        size_t index = self->bestTriangleBandMatches[i];
                        self->segmentLengths[i] = Matrix_getRow(self->paletteData->beats, 1)[index];
                        self->bestTriangleBandMatches[i] = Matrix_getRow(self->paletteData->beats, 0)[index];
                    }
                }
                
                AudioAnalyser32_findMagnitudeDifferences(self->audioAnalyser,
                                                         self->analysisQueueComparisonData,
                                                         self->audioAnalyser->paletteComparisonData,
                                                         self->segmentLengths,
                                                         self->bestTriangleBandMatches,
                                                         self->segmentMagnitudeDifferences);
                
                
                CsoundObject_writeOpenCLPVSReadPath(self->csoundObject,
                                                    self->paletteData->triangleMagnitudeBandsCount,
                                                    self->bestTriangleBandMatches,
                                                    self->segmentLengths,
                                                    self->segmentMagnitudeDifferences,
                                                    self->audioAnalyser->triangleBandGains);
            }
        }
        
        CsoundObject_readCallback(self->csoundObject,
                                  self->samplesPerFrame,
                                  self->stereoBuffer);
        
        AudioIOProcess32_writeAudioToOutput(self,
                                            self->stereoBuffer,
                                            ioData,
                                            currentFrame);
    }
    
    return noErr;
}

void AudioIOProcess32_configureAudioUnit(AudioIOProcess32 *self)
{
    OSStatus result = noErr;
    
    AudioComponentDescription outputComponentDescription = {
        
        .componentType = kAudioUnitType_Output,
        .componentSubType = kAudioUnitSubType_DefaultOutput,
        .componentManufacturer = kAudioUnitManufacturer_Apple
    };
    
    AudioComponent audioComponent = AudioComponentFindNext (NULL, &outputComponentDescription);
    
    if (audioComponent == NULL) {
        
        printf ("can't get output unit");
        exit (-1);
    }
    
    result = AudioComponentInstanceNew(audioComponent,
                                       &self->outputUnit);
    
    CheckError (result, "Couldn't open component for outputUnit");
    
    AURenderCallbackStruct renderCallbackStruct = {
        
        .inputProc = AudioIOProcess32_RenderProc,
        .inputProcRefCon = self
    };
    
    result = AudioUnitSetProperty(self->outputUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input,
                                  0,
                                  &renderCallbackStruct,
                                  sizeof(renderCallbackStruct));
    
    CheckError (result, "AudioUnitSetProperty kAudioUnitProperty_SetRenderCallback failed");
    
    result = AudioUnitInitialize(self->outputUnit);
    
    CheckError (result, "Couldn't initialize output unit");
}

void AudioIOProcess32_configureCsound(AudioIOProcess32 *self)
{
    self->csoundObject = CsoundObject_new("Streaming-Audio-Mosaicing-Vocoder/Csound/SynthesisProcess.csd", self->maximumSegmentFrameCount);
    
    
    CsoundObject_createPVSFile(self->csoundObject, self->paletteAudioObject->filePath);
    Float32 *blankArray = calloc(self->maximumSegmentFrameCount, sizeof(Float32));
    
    for (size_t i = 0; i < self->paletteData->triangleMagnitudeBandsCount; ++i) {
        
        CsoundObject_writeDataToTable(self->csoundObject,
                                      (Float32)warpPathsBaseTableNumber + i,
                                      blankArray,
                                      (UInt32)self->maximumSegmentFrameCount);
        
        CsoundObject_writeDataToTable(self->csoundObject,
                                      (Float32)bandGainsBaseTableNumber + i,
                                      Matrix_getRow(self->audioAnalyser->triangleBandGains,i),
                                      (UInt32)self->FFTFrameSize/2);
    }
    
    CsoundObject_turnOnPhaseVocoder(self->csoundObject,
                                    self->paletteData->triangleMagnitudeBandsCount);
    
    
    free(blankArray);
}


void AudioIOProcess32_process(AudioIOProcess32 *self,
                              AudioObject *audioObject,
                              UInt32 time)
{
    self->audioObject = audioObject;
    
    AudioOutputUnitStart(self->outputUnit);
    
    sleep(time);
    
    AudioOutputUnitStop(self->outputUnit);
}

void AudioIOProcess32_processToAudioObject(AudioIOProcess32 *self,
                                           AudioObject *analysisAudioObject,
                                           AudioObject *saveFileAudioObject)
{
    self->audioObject = analysisAudioObject;
    
    while(self->audioObject->currentFramePosition < self->audioObject->frameCount - self->FFTFrameSize
          &&
          saveFileAudioObject->currentFramePosition < saveFileAudioObject->frameCount -  - self->FFTFrameSize){
        
        AudioObject_readCallbackMono(self->audioObject,
                                     self->samplesPerFrame,
                                     self->monoBuffer);
        
        RingBufferFloat32_writeCopySkip(self->ringBuffer,
                                        self->monoBuffer,
                                        self->hopSize,
                                        self->frameBuffer,
                                        self->FFTFrameSize,
                                        self->hopSize);
        
        if (self->frameCount >= 3) {
            
            AudioAnalyser32_analyseAudioFrameToQueue(self->audioAnalyser,
                                                     self->frameBuffer,
                                                     self->analysisQueue,
                                                     self->paletteData);
            
            if (self->analysisQueue->currentFrame == 0) {
                
                
                AudioAnalyser32_findMatchOpenCL(self->audioAnalyser,
                                                self->analysisQueueComparisonData,
                                                self->warpFrameTimesInSeconds->data,
                                                self->bestTriangleBandMatches);
                
                if (self->useBeats) {
                    
                    for (size_t i = 0; i < self->paletteData->triangleMagnitudeBandsCount; ++i) {
                        
                        size_t index = self->bestTriangleBandMatches[i];
                        self->segmentLengths[i] = Matrix_getRow(self->paletteData->beats, 1)[index];
                        self->bestTriangleBandMatches[i] = Matrix_getRow(self->paletteData->beats, 0)[index];
                    }
                }
                
                AudioAnalyser32_findMagnitudeDifferences(self->audioAnalyser,
                                                         self->analysisQueueComparisonData,
                                                         self->audioAnalyser->paletteComparisonData,
                                                         self->segmentLengths,
                                                         self->bestTriangleBandMatches,
                                                         self->segmentMagnitudeDifferences);
                
                
                CsoundObject_writeOpenCLPVSReadPath(self->csoundObject,
                                                    self->paletteData->triangleMagnitudeBandsCount,
                                                    self->bestTriangleBandMatches,
                                                    self->segmentLengths,
                                                    self->segmentMagnitudeDifferences,
                                                    self->audioAnalyser->triangleBandGains);
            }
            
        }
        self->frameCount++;

        CsoundObject_readCallback(self->csoundObject,
                                  self->samplesPerFrame,
                                  self->stereoBuffer);
        
         AudioObject_writeBuffersOfSamples(saveFileAudioObject, 2, self->stereoBuffer, self->samplesPerFrame);
    }
}

