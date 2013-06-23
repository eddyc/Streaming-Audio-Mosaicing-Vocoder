//
//  AnalysisData.c
//  ConcatenationRewrite
//
//  Created by Edward Costello on 23/01/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import "AudioAnalysisQueue.h"
#import <Accelerate/Accelerate.h>
#import <stdio.h>
#import <string.h>
#import "hdf5.h"

AudioAnalysisQueue32 *AudioAnalysisQueue32_new(size_t FFTFrameSize,
                                               size_t frameCount,
                                               size_t triangleFilterSize,
                                               size_t *triangleRowBlockSizes,
                                               size_t triangleMagnitudeBandsCount)
{
    AudioAnalysisQueue32 *self = calloc(1, sizeof(AudioAnalysisQueue32));
    self->frameCount = frameCount;
    self->chromagram = Matrix32_new(self->frameCount, 12);
    self->mfccs = Matrix32_new(self->frameCount, 13);
    self->magnitudes = Matrix32_new(self->frameCount, FFTFrameSize/2);
    self->triangleFilteredMagnitudes = Matrix32_new(self->frameCount, triangleFilterSize);
    self->triangleFluxFilteredMagnitudes = Matrix32_new(self->frameCount, triangleFilterSize);

    self->triangleMagnitudeBandsCount = triangleMagnitudeBandsCount;
    
    self->triangleMagnitudeBands = calloc(self->triangleMagnitudeBandsCount, sizeof(Matrix32 *));
    self->triangleFluxMagnitudeBands = calloc(self->triangleMagnitudeBandsCount, sizeof(Matrix32 *));

    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        self->triangleMagnitudeBands[i] = Matrix32_new(self->frameCount, triangleRowBlockSizes[i]);
        self->triangleFluxMagnitudeBands[i] = Matrix32_new(self->frameCount, triangleRowBlockSizes[i]);
    }
    
    self->currentFrame = 0;
    return self;
}

void AudioAnalysisQueue32_delete(AudioAnalysisQueue32 *self)
{
    Matrix32_delete(self->chromagram);
    Matrix32_delete(self->magnitudes);
    Matrix32_delete(self->mfccs);
    Matrix32_delete(self->triangleFilteredMagnitudes);
    Matrix32_delete(self->triangleFluxFilteredMagnitudes);

    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        Matrix32_delete(self->triangleMagnitudeBands[i]);
        Matrix32_delete(self->triangleFluxMagnitudeBands[i]);
    }
    
    free(self);
    self = NULL;
}

static void AudioAnalysisQueue32_addDataSetToHDF(Matrix32 *self, hid_t *fileID, char *dataSet)
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

void AudioAnalysisQueue32_toHDF(AudioAnalysisQueue32 *self, char *path)
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
    char *triangleFilteredMagnitudesDataSet = "/triangleFilteredMagnitudes";
    char *triangleFluxFilteredMagnitudesDataSet = "/triangleFluxFilteredMagnitudes";

    char triangleBandSet[50];
    char triangleFluxBandSet[50];

    
    for (size_t i = 0; i < self->triangleMagnitudeBandsCount; ++i) {
        
        sprintf(triangleBandSet, "/triangleBand%zd", i);
        AudioAnalysisQueue32_addDataSetToHDF(self->triangleMagnitudeBands[i], &fileID, triangleBandSet);
        
        sprintf(triangleBandSet, "/triangleFluxBand%zd", i);
        AudioAnalysisQueue32_addDataSetToHDF(self->triangleFluxMagnitudeBands[i], &fileID, triangleFluxBandSet);
    }
    
    AudioAnalysisQueue32_addDataSetToHDF(self->mfccs, &fileID, mfccDataSet);
    AudioAnalysisQueue32_addDataSetToHDF(self->chromagram, &fileID, chromagramDataSet);
    AudioAnalysisQueue32_addDataSetToHDF(self->magnitudes, &fileID, magnitudesDataSet);
    AudioAnalysisQueue32_addDataSetToHDF(self->triangleFilteredMagnitudes, &fileID, triangleFilteredMagnitudesDataSet);
    AudioAnalysisQueue32_addDataSetToHDF(self->triangleFluxFilteredMagnitudes, &fileID, triangleFluxFilteredMagnitudesDataSet);

    status = H5Fclose(fileID);
}

