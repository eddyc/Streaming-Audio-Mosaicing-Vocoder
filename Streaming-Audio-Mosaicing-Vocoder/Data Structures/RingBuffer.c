//
//  RingBufferFloat32.c
//  ConcatenationRewrite
//
//  Created by Edward Costello on 26/01/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import "RingBuffer.h"
#import "MathematicalFunctions.h"
#import <stdio.h>
#import <Accelerate/Accelerate.h>

RingBufferFloat32 *RingBufferFloat32_new(size_t capacity)
{
    RingBufferFloat32 *self = calloc(1, sizeof(RingBufferFloat32));
    self->capacity = nextPowerOfTwo(capacity);
    self->dataFreeCount = self->capacity;
    self->data = calloc(self->capacity, sizeof(Float32));
    
    return self;
}

void RingBufferFloat32_delete(RingBufferFloat32 *self)
{
    free(self->data);
    free(self);
    self = NULL;
}

static inline void RingBufferFloat32_checkDataSize(RingBufferFloat32 *self, size_t dataSize)
{
    if (dataSize > self->capacity) {
        
        printf("RingBufferFloat32 error, requested data size is larger than buffer size, exiting\n");
        exit(-1);
    }
}

static void RingBufferFloat32_setDataSize(RingBufferFloat32 *self, SInt64 dataSize)
{
    self->dataFreeCount -= dataSize;
    self->dataWrittenCount += dataSize;
}

inline static Boolean RingBufferFloat32_checkForWrapAround(RingBufferFloat32 *self, size_t pointer, size_t dataSize)
{
    return ((dataSize + pointer) > self->capacity);
}

inline void RingBufferFloat32_write(RingBufferFloat32 *self, Float32 *__restrict inputData, size_t inputDataSize)
{
    if (self->dataWrittenCount >= self->capacity) {
        
        return;
    }
    
    RingBufferFloat32_checkDataSize(self, inputDataSize);
    
    if (RingBufferFloat32_checkForWrapAround(self, self->writePointer, inputDataSize) == true) {
        
        size_t firstHalfSize = self->capacity - self->writePointer;
        size_t secondHalfSize = inputDataSize - firstHalfSize;
        
        cblas_scopy((SInt32)firstHalfSize, inputData, 1, &self->data[self->writePointer], 1);
        self->writePointer = 0;
        cblas_scopy((SInt32)secondHalfSize, &inputData[firstHalfSize], 1, &self->data[self->writePointer], 1);
        self->writePointer += secondHalfSize;
    }
    else {
        
        cblas_scopy((SInt32)inputDataSize, inputData, 1, &self->data[self->writePointer], 1);
        self->writePointer += inputDataSize;
    }
    
    self->writePointer &= self->capacity - 1;
    self->readPointer += inputDataSize;
    self->readPointer &= self->capacity - 1;
    
    RingBufferFloat32_setDataSize(self, inputDataSize);
}

inline void RingBufferFloat32_copy(RingBufferFloat32 *self, Float32 *__restrict outputData, size_t outputDataSize)
{
    RingBufferFloat32_checkDataSize(self, outputDataSize);
    
    if (RingBufferFloat32_checkForWrapAround(self, self->readPointer, outputDataSize) == true) {
        
        size_t firstHalfSize = self->capacity - self->readPointer;
        size_t secondHalfSize = outputDataSize - firstHalfSize;
        
        cblas_scopy((SInt32)firstHalfSize, &self->data[self->readPointer], 1, outputData, 1);
        cblas_scopy((SInt32)secondHalfSize, &self->data[0], 1, &outputData[firstHalfSize], 1);
        
    }
    else {
        
        cblas_scopy((SInt32)outputDataSize, &self->data[self->readPointer], 1, outputData, 1);
    }
}


inline void RingBufferFloat32_skip(RingBufferFloat32 *self, size_t skipCount)
{
    if (self->dataFreeCount == self->capacity) {
        
        return;
    }
    
    RingBufferFloat32_checkDataSize(self, skipCount);
    
    RingBufferFloat32_setDataSize(self,-(SInt32)skipCount);
}

inline void RingBufferFloat32_writeCopySkip(RingBufferFloat32 *self,
                                            Float32 *__restrict inputData,
                                            size_t inputDataSize,
                                            Float32 *__restrict outputData,
                                            size_t outputDataSize,
                                            size_t skipCount)
{
    RingBufferFloat32_write(self, inputData, inputDataSize);
    RingBufferFloat32_copy(self, outputData, outputDataSize);
    RingBufferFloat32_skip(self, skipCount);
}

