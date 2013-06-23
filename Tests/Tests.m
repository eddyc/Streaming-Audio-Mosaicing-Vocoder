//
//  Tests.m
//  Tests
//
//  Created by Edward Costello on 13/03/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import "Tests.h"
#import "AudioObject.h"
#import "AudioAnalyser.h"
#import "AudioIOProcess.h"
#import "ConvenienceFunctions.h"
#import "OpenCLDTW.h"
#import "CsoundObject.h"
#import "TriangleFilterBank.h"

@implementation Tests


- (void)testImplementation
{

    AudioObject *paletteAudioObject = AudioObject_new();
    AudioObject_openAudioFile(paletteAudioObject, "Streaming-Audio-Mosaicing-Vocoder/Audio-Files/input.wav");
    AudioObject *analysisAudioObject = AudioObject_new();
    AudioObject_openAudioFile(analysisAudioObject, "Streaming-Audio-Mosaicing-Vocoder/Audio-Files/sava.wav");
    AudioObject *saveFileAudioObject = AudioObject_newBlank(2, analysisAudioObject->frameCount);
    
    size_t FFTFrameSize = 1024;
    size_t hopSize = FFTFrameSize / 4;
    size_t triangleFilterCount = 30;
    size_t triangleBandsCount = 20;
    
    paletteAudioObject->frameCount = paletteAudioObject->frameCount;
    analysisAudioObject->frameCount = analysisAudioObject->frameCount;
    
    AudioAnalyser32 *audioAnalyser = AudioAnalyser32_new(paletteAudioObject->samplerate,
                                                         FFTFrameSize,
                                                         hopSize,
                                                         triangleFilterCount,
                                                         triangleBandsCount);
    
    AudioAnalysisData32 *paletteAnalysisData = AudioAnalysisData32_new(paletteAudioObject->samplerate,
                                                                       paletteAudioObject->frameCount,
                                                                       FFTFrameSize,
                                                                       hopSize,
                                                                       triangleFilterCount,
                                                                       triangleBandsCount);
    
    AudioAnalyser32_analyseAudioObject(audioAnalyser,
                                       paletteAudioObject,
                                       paletteAnalysisData, 240);
    
    AudioIOProcess32 *audioProcess = AudioIOProcess32_new(audioAnalyser,
                                                          paletteAudioObject,
                                                          paletteAnalysisData,
                                                          2,
                                                          false,
                                                          false);
    
    //    AudioIOProcess32_process(audioProcess, analysisAudioObject, 200);
    AudioIOProcess32_processToAudioObject(audioProcess, analysisAudioObject, saveFileAudioObject);
    AudioObject_saveAudioFile(saveFileAudioObject, "Streaming-Audio-Mosaicing-Vocoder/Audio-Files/Tests/test.wav");
    
    AudioIOProcess32_delete(audioProcess);
    
    AudioAnalysisData32_delete(paletteAnalysisData);
    AudioAnalyser32_delete(audioAnalyser);
    AudioObject_delete(paletteAudioObject);
    AudioObject_delete(analysisAudioObject);
    AudioObject_delete(saveFileAudioObject);
    
}


@end
