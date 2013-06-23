//
//  AnalysisData.c
//  ConcatenationRewrite
//
//  Created by Edward Costello on 23/01/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import "AudioAnalysisData.h"
#import "ConvenienceFunctions.h"
#import <Accelerate/Accelerate.h>
#import <stdio.h>
#import <string.h>
#import "hdf5.h"

AudioAnalysisData32 *AudioAnalysisData32_new(size_t samplerate,
                                             size_t sampleCount,
                                             size_t FFTFrameSize,
                                             size_t hopSize,
                                             size_t triangleMagnitudesCount,
                                             size_t triangleMagnitudeBandsCount)
{
    AudioAnalysisData32 *self = calloc(1, sizeof(AudioAnalysisData32));
    self->sampleCount = sampleCount;
    self->FFTFrameSize = FFTFrameSize;
    self->hopSize = hopSize;
    self->triangleMagnitudeBandsCount = triangleMagnitudeBandsCount;
    self->hopCount = 1 + floor((self->sampleCount - self->FFTFrameSize/2)/self->hopSize);
    
    self->chromagram = Matrix32_new(self->hopCount, 12);
    self->mfccs = Matrix32_new(self->hopCount, 13);
    self->magnitudes = Matrix32_new(self->hopCount, self->FFTFrameSize/2);
    self->beats = Matrix32_new(2, self->hopCount);
    self->triangleMagnitudes = Matrix32_new(self->hopCount, triangleMagnitudesCount);
    self->triangleFluxMagnitudes = Matrix32_new(self->hopCount, triangleMagnitudesCount);

    AudioAnalysisData32_calculateTriangleFilterBandSizes(self);

    self->frameTimeInSeconds = calloc(self->hopCount, sizeof(Float32));
    self->samplerate = samplerate;
    
    for (size_t i = 0; i < self->hopCount; ++i) {
        
        Float32 currentTime = (Float32)i * ((Float32)self->hopSize / (Float32)self->samplerate);
        self->frameTimeInSeconds[i] = currentTime;
    }

    return self;
}

void AudioAnalysisData32_delete(AudioAnalysisData32 *self)
{
    Matrix32_delete(self->chromagram);
    Matrix32_delete(self->mfccs);
    Matrix32_delete(self->magnitudes);
    Matrix32_delete(self->beats);
    Matrix32_delete(self->triangleMagnitudes);
    Matrix32_delete(self->triangleFluxMagnitudes);

    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        Matrix32_delete(self->triangleMagnitudeBands[i]);
        Matrix32_delete(self->triangleFluxMagnitudeBands[i]);

    }
    
    free(self->triangleMagnitudeBands);
    free(self->triangleRowBlockSizes);
    free(self);
    self = NULL;
}

void AudioAnalysisData32_calculateTriangleFilterBandSizes(AudioAnalysisData32 *self)
{
    Float32 averageBlockSize = (Float32)self->triangleMagnitudes->columnCount / (Float32)self->triangleMagnitudeBandsCount;
    self->largestTriangleBandSize = ceilf(averageBlockSize);
    self->triangleRowBlockSizes = calloc(self->triangleMagnitudeBandsCount, sizeof(size_t));
    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        self->triangleRowBlockSizes[i] = roundf(averageBlockSize * (i + 1));
    }
    
    for (size_t i = self->triangleMagnitudeBandsCount - 1; i > 0; --i) {
        
        self->triangleRowBlockSizes[i] -= self->triangleRowBlockSizes[i - 1];
    }

    self->triangleMagnitudeBands = calloc(self->triangleMagnitudeBandsCount, sizeof(Matrix32 *));
    self->triangleFluxMagnitudeBands = calloc(self->triangleMagnitudeBandsCount, sizeof(Matrix32 *));

    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        self->triangleMagnitudeBands[i] = Matrix32_new(self->hopCount, self->triangleRowBlockSizes[i]);
        self->triangleFluxMagnitudeBands[i] = Matrix32_new(self->hopCount, self->triangleRowBlockSizes[i]);
    }
}

static void AudioAnalysisData32_addDataSetToHDF(Matrix32 *self, hid_t *fileID, char *dataSet)
{
    hid_t dataSetID, dataSpaceID;
    hsize_t dimensions[2];
    herr_t status;

    dimensions[0] = self->rowCount;
    dimensions[1] = self->columnCount;
    
    dataSpaceID = H5Screate_simple(2, dimensions, NULL);
    
    dataSetID = H5Dcreate(*fileID,
                          dataSet,
                          H5T_NATIVE_FLOAT,
                          dataSpaceID,
                          H5P_DEFAULT,
                          H5P_DEFAULT,
                          H5P_DEFAULT);
    
    H5Dwrite (dataSetID,
              H5T_NATIVE_FLOAT,
              H5S_ALL,
              H5S_ALL,
              H5P_DEFAULT,
              self->data);
    
    status = H5Dclose(dataSetID);
    status = H5Sclose(dataSpaceID);
}

void AudioAnalysisData32_saveToHDF(AudioAnalysisData32 *self, char *path)
{
    hid_t fileID;
    herr_t status;
    

    fileID = H5Fcreate(path,
                       H5F_ACC_TRUNC,
                       H5P_DEFAULT,
                       H5P_DEFAULT);
    
    char *mfccDataSet = "/mfcc";
    char *chromagramDataSet = "/chromagram";
    char *magnitudesDataSet = "/magnitudes";
    char *beatsDataSet = "/beats";
    char *triangleFilteredMagnitudesDataSet = "/triangleFilteredMagnitudes";
    char triangleBandSet[50];
    char *triangleFluxFilteredMagnitudesDataSet = "/triangleFluxFilteredMagnitudes";
    char triangleFluxBandSet[50];

    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        sprintf(triangleBandSet, "/triangleBand%zd", i);
        AudioAnalysisData32_addDataSetToHDF(self->triangleMagnitudeBands[i], &fileID, triangleBandSet);
        sprintf(triangleFluxBandSet, "/triangleFluxBand%zd", i);
        AudioAnalysisData32_addDataSetToHDF(self->triangleFluxMagnitudeBands[i], &fileID, triangleFluxBandSet);
    }
    
    AudioAnalysisData32_addDataSetToHDF(self->mfccs, &fileID, mfccDataSet);
    AudioAnalysisData32_addDataSetToHDF(self->chromagram, &fileID, chromagramDataSet);
    AudioAnalysisData32_addDataSetToHDF(self->magnitudes, &fileID, magnitudesDataSet);
    AudioAnalysisData32_addDataSetToHDF(self->beats, &fileID, beatsDataSet);
    AudioAnalysisData32_addDataSetToHDF(self->triangleMagnitudes, &fileID, triangleFilteredMagnitudesDataSet);
    AudioAnalysisData32_addDataSetToHDF(self->triangleFluxMagnitudes, &fileID, triangleFluxFilteredMagnitudesDataSet);
    status = H5Fclose(fileID);
}

