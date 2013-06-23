//
//  RingBufferFloat32.h
//  ConcatenationRewrite
//
//  Created by Edward Costello on 26/01/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import <MacTypes.h>
#import <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif
    
    typedef struct RingBufferFloat32
    {
        size_t capacity;
        
        size_t dataWrittenCount;
        size_t dataFreeCount;

        size_t readPointer;
        size_t writePointer;
        
        Float32 *data;
        
    } RingBufferFloat32;
    
    RingBufferFloat32 *RingBufferFloat32_new(size_t capacity);
    void RingBufferFloat32_delete(RingBufferFloat32 *);
    
    extern inline void RingBufferFloat32_write(RingBufferFloat32 *self,
                                               Float32 *__restrict inputData,
                                               size_t inputDataSize);
    
    extern inline void RingBufferFloat32_copy(RingBufferFloat32 *self,
                                              Float32 *__restrict outputData,
                                              size_t outputDataSize);
    
    extern inline void RingBufferFloat32_skip(RingBufferFloat32 *self,
                                              size_t skipCount);
    
    extern inline void RingBufferFloat32_writeCopySkip(RingBufferFloat32 *self,
                                                       Float32 *__restrict inputData,
                                                       size_t inputDataSize,
                                                       Float32 *__restrict outputData,
                                                       size_t outputDataSize,
                                                       size_t skipCount);



    
#ifdef __cplusplus
}
#endif