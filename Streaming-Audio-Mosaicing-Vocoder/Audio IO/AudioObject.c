//
//  AudioObject.c
//  Audio IO Tester
//
//  Created by Edward Costello on 29/11/2012.
//  Copyright (c) 2012 Edward Costello. All rights reserved.
//

#import "AudioObject.h"


static void checkError(OSStatus error, const char *operation)
{
	if (error == noErr) return;
	
	char str[20];
	// see if it appears to be a 4-char-code
	*(UInt32 *)&str[1] = CFSwapInt32HostToBig(error);
    
	if (isprint(str[1]) && isprint(str[2]) && isprint(str[3]) && isprint(str[4])) {
		str[0] = str[5] = '\'';
		str[6] = '\0';
	} else
		// no, format it as an integer
		sprintf(str, "%d", (int)error);
    
    printf("Error: %s failed (%s)\n", operation, str);
    
    exit(1);
}

AudioObject *AudioObject_new()
{
    AudioObject *self = calloc(1, sizeof(AudioObject));
    
    self->fileStreamDescription = calloc(1, sizeof(AudioStreamBasicDescription));
    self->processStreamDescription = calloc(1, sizeof(AudioStreamBasicDescription));
    return self;
}

Float32 *AudioObject_reallocateArray(Float32 *array, size_t oldSize, size_t newSize)
{
    if (newSize == oldSize) {
        
        return array;
    }
    else if (oldSize > newSize) {
        
        Float32 *newArray = (Float32 *)malloc(sizeof(Float32) * newSize);
        memcpy(newArray, array, sizeof(Float32) * newSize);
        
        
        return newArray;
    }
    else if (newSize > oldSize) {
        
        Float32 *newArray = (Float32 *)calloc(newSize, sizeof(Float32));
        memcpy(newArray, array, sizeof(Float32) * oldSize);
        return newArray;
    }
    
    return array;
}


AudioObject *AudioObject_newBlank(UInt8 channelCount, size_t frameCount)
{
    AudioObject *self = AudioObject_new();
    
    AudioObject_setupCanonicalAudioStreamBasicDescription(self, kFileStreamDescription);
    AudioObject_setupCanonicalAudioStreamBasicDescription(self, kProcessStreamDescription);
    
    self->fileStreamDescription->mChannelsPerFrame = channelCount;
    self->processStreamDescription->mChannelsPerFrame = channelCount;
    self->writeIndex = calloc(channelCount, sizeof(size_t));
    self->channelCount = channelCount;
    self->frameCount = frameCount;
    self->audioChannels = calloc(channelCount, sizeof(Float32 **));
    
    for (size_t i = 0; i < channelCount; ++i) {
        
        self->audioChannels[i] = calloc(frameCount, sizeof(Float32));
    }
    
    if (channelCount == 1) {
        
        self->channelMono = self->audioChannels[0];
    }
    
    self->populated = true;
    
    return self;
}

void AudioObject_writeBufferOfSamples(AudioObject *self, size_t channel, Float32 *buffer, size_t bufferSize)
{
    if (self->frameCount + bufferSize >= self->audioChannelCapacity) {
        
        size_t newSize = (self->audioChannelCapacity + bufferSize) * 2;
        
        for (UInt8 channel = 0; channel < self->channelCount; ++channel) {
            
            self->audioChannels[channel] = AudioObject_reallocateArray(self->audioChannels[channel], self->audioChannelCapacity, newSize);
        }
        
        if (self->channelCount >= 2) {
            
            self->channelMono = AudioObject_reallocateArray(self->channelMono, self->audioChannelCapacity, newSize);
        }
        
        self->audioChannelCapacity = newSize;
    }
    
    memcpy(&self->audioChannels[channel][self->writeIndex[channel]], buffer, sizeof(Float32) * bufferSize);
    Float32 scalar = 1. / self->channelCount;
    vDSP_vsma(buffer, 1, &scalar, &self->channelMono[self->writeIndex[channel]], 1, &self->channelMono[self->writeIndex[channel]], 1, bufferSize);
    
    self->writeIndex[channel] += bufferSize;
    self->frameCount = self->writeIndex[channel];
    
}

void AudioObject_writeBuffersOfSamples(AudioObject *self, size_t channelCount, Float32 **buffers, size_t bufferSizes)
{
    for (size_t channel = 0; channel < channelCount; ++channel) {
        
        AudioObject_writeBufferOfSamples(self, channel, buffers[channel], bufferSizes);
    }
}


void AudioObject_delete(AudioObject *self)
{
    if (self->channelCount >= 2) {
        
        if (self->channelMono != NULL) {
            
            free(self->channelMono);
        }
    }
    
    for (size_t i = 0; i < self->channelCount; ++i) {
        
        free(self->audioChannels[i]);
    }
    
    free(self->audioChannels);
    free(self->fileStreamDescription);
    free(self->processStreamDescription);
    free(self);
    self = NULL;
}

void AudioObject_setupCanonicalAudioStreamBasicDescription(AudioObject *self, CanonicalAudioStreamBasicDescription description)
{
    if (description == kProcessStreamDescription) {
        
        self->processStreamDescription->mSampleRate = 44100.;
        self->processStreamDescription->mFormatID = kAudioFormatLinearPCM;
        self->processStreamDescription->mFormatFlags = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsNonInterleaved;
        self->processStreamDescription->mBytesPerPacket = 4;
        self->processStreamDescription->mFramesPerPacket = 1;
        self->processStreamDescription->mBytesPerFrame = 4;
        self->processStreamDescription->mChannelsPerFrame = self->channelCount = 2;
        self->processStreamDescription->mBitsPerChannel = 32;
    }
    if (description == kFileStreamDescription) {
        
        self->fileStreamDescription->mSampleRate = 44100.;
        self->fileStreamDescription->mFormatID = kAudioFormatLinearPCM;
        self->fileStreamDescription->mFormatFlags = 12;
        self->fileStreamDescription->mBytesPerPacket = 4;
        self->fileStreamDescription->mFramesPerPacket = 1;
        self->fileStreamDescription->mBytesPerFrame = 4;
        self->fileStreamDescription->mChannelsPerFrame = self->channelCount = 2;
        self->fileStreamDescription->mBitsPerChannel = 16;
    }
}



void AudioObject_openAudioFile(AudioObject *self, const char *path)
{
    self->filePath = (char *)path;
    OSStatus result = noErr;
    ExtAudioFileRef inputAudioFileReference = {0};
    
    CFStringRef pathAsCFString = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    
    CFURLRef fileURLRef = CFURLCreateWithFileSystemPath(NULL, pathAsCFString, kCFURLPOSIXPathStyle, false);
    
    result = ExtAudioFileOpenURL(fileURLRef,
                                 &inputAudioFileReference);
    
    checkError(result, "ExtAudioFileOpenURL");
    
    
    UInt32 audioStreamDescriptionSize = sizeof(*self->fileStreamDescription);
    
    result = ExtAudioFileGetProperty(inputAudioFileReference,
                                     kExtAudioFileProperty_FileDataFormat,
                                     &audioStreamDescriptionSize,
                                     self->fileStreamDescription);
    
    checkError(result, "ExtAudioFileGetProperty kExtAudioFileProperty_FileDataFormat");
    
    self->samplerate = self->fileStreamDescription->mSampleRate;
    
    self->processStreamDescription->mChannelsPerFrame = self->fileStreamDescription->mChannelsPerFrame;
    self->processStreamDescription->mBytesPerFrame = 4;
    self->processStreamDescription->mBytesPerPacket = 4;
    self->processStreamDescription->mFramesPerPacket = 1;
    self->processStreamDescription->mBitsPerChannel = 32;
    self->processStreamDescription->mFormatID = kAudioFormatLinearPCM;
    self->processStreamDescription->mSampleRate = self->samplerate;
    self->processStreamDescription->mFormatFlags = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsNonInterleaved;
    
    
    
    result = ExtAudioFileSetProperty(inputAudioFileReference,
                                     kExtAudioFileProperty_ClientDataFormat,
                                     sizeof(*self->processStreamDescription),
                                     self->processStreamDescription);
    
    checkError(result, "ExtAudioFileSetProperty kExtAudioFileProperty_ClientDataFormat");
    
    
    SInt64 frameCount;
    UInt32 frameCountSize = sizeof(SInt64);
    
    result = ExtAudioFileGetProperty(inputAudioFileReference,
                                     kExtAudioFileProperty_FileLengthFrames,
                                     &frameCountSize,
                                     &frameCount);
    
    
    checkError(result, "ExtAudioFileGetProperty kExtAudioFileProperty_FileLengthFrames");
    
    
    self->channelCount = self->fileStreamDescription->mChannelsPerFrame;
    self->frameCount = frameCount;
    self->audioChannels = calloc(self->channelCount, sizeof(AudioSampleType *));
    self->audioChannelCapacity = frameCount;
    self->writeIndex = calloc(self->channelCount, sizeof(size_t));
    
    
    AudioBufferList *audioBufferList = (AudioBufferList *)malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer) * (self->fileStreamDescription->mChannelsPerFrame - 1));
    
    audioBufferList->mNumberBuffers = self->fileStreamDescription->mChannelsPerFrame;
    
    for (UInt32 i = 0; i < audioBufferList->mNumberBuffers; ++i) {
        
        audioBufferList->mBuffers[i].mDataByteSize = (UInt32) (sizeof(Float32) * frameCount);
        audioBufferList->mBuffers[i].mNumberChannels = 1;
        
        self->audioChannels[i] = (AudioSampleType *) calloc(frameCount, sizeof(AudioSampleType));
        audioBufferList->mBuffers[i].mData = self->audioChannels[i];
        self->writeIndex[i] = frameCount;
    }
    
    
    
    UInt32 frameCountUInt32 = (UInt32) frameCount;
    
    result = ExtAudioFileRead(inputAudioFileReference,
                              &frameCountUInt32,
                              audioBufferList);
    
    checkError(result, "ExtAudioFileRead");
    
    if (self->channelCount >= 2) {
        
        self->channelMono = (AudioSampleType *) calloc(frameCount, sizeof(AudioSampleType));
        
        for (size_t i = 0; i < self->channelCount; ++i) {
            
            for (size_t j = 0; j < self->frameCount; ++j) {
                
                self->channelMono[j] += self->audioChannels[i][j] / (Float32)self->channelCount;
            }
        }
    }
    else {
        
        self->channelMono = self->audioChannels[1] = self->audioChannels[0];
    }
    
    
    result = ExtAudioFileDispose(inputAudioFileReference);
    
    checkError(result, "ExtAudioFileDispose");
    
    if (self->fileStreamDescription->mFormatID != kAudioFormatLinearPCM) {
        
        AudioObject_setupCanonicalAudioStreamBasicDescription(self, kFileStreamDescription);
    }
    
    
    free(audioBufferList);
    audioBufferList = NULL;
    
    self->populated = true;
}

void AudioObject_saveAudioFile(AudioObject *self, const char *path)
{
    OSStatus result = noErr;
    CFStringRef pathAsCFString = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    
    CFURLRef fileURLRef = CFURLCreateWithFileSystemPath(NULL, pathAsCFString, kCFURLPOSIXPathStyle, false);
    
    ExtAudioFileRef outputAudioFileReference;
    AudioFileTypeID outputAudioFileTypeID = kAudioFileWAVEType;
    
    
    result = ExtAudioFileCreateWithURL(fileURLRef,
                                       outputAudioFileTypeID,
                                       self->fileStreamDescription,
                                       NULL,
                                       kAudioFileFlags_EraseFile,
                                       &outputAudioFileReference);
    
    
    checkError(result, "ExtAudioFileCreateWithURL");
    
    result = ExtAudioFileSetProperty(outputAudioFileReference,
                                     kExtAudioFileProperty_ClientDataFormat,
                                     sizeof(*self->processStreamDescription),
                                     self->processStreamDescription);
    
    checkError(result, "ExtAudioFileSetProperty kExtAudioFileProperty_ClientDataFormat");
    
    
    
    
    AudioBufferList *audioBufferList = (AudioBufferList *)malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer) * (self->processStreamDescription->mChannelsPerFrame - 1));
    
    audioBufferList->mNumberBuffers = self->processStreamDescription->mChannelsPerFrame;
    
    for (UInt32 i = 0; i < audioBufferList->mNumberBuffers; ++i) {
        
        audioBufferList->mBuffers[i].mDataByteSize = (UInt32) (sizeof(AudioSampleType) * self->frameCount);
        audioBufferList->mBuffers[i].mNumberChannels = 1;
        audioBufferList->mBuffers[i].mData = self->audioChannels[i];
        
    }
    
    result = ExtAudioFileWrite(outputAudioFileReference,
                               (UInt32)self->frameCount,
                               audioBufferList);
    
    checkError(result, "ExtAudioFileWrite");
    
    ExtAudioFileDispose(outputAudioFileReference);
    
    free(audioBufferList);
    
    audioBufferList = NULL;
    
}

void AudioObject_readSamples(AudioObject *self,
                             SInt32 readOffset,
                             SInt32 channel,
                             Float32 *output,
                             size_t outputSize)
{
    
    Float32 *audioChannel;
    
    if (channel < self->channelCount) {
        
        if (channel == -1) {
            
            audioChannel = self->channelMono;
        }
        else {
            
            audioChannel = self->audioChannels[channel];
        }
    }
    
    if (readOffset < 0) {
        
        if ((readOffset + (SInt32)outputSize) < 0) {
            
            vDSP_vclr(output, 1, outputSize);
            
            return;
        }
        else if ((readOffset + (SInt32)outputSize) >= 0
                 &&
                 (readOffset + (SInt32)outputSize) < self->frameCount) {
            
            SInt32 negativeCount = -readOffset;
            SInt32 positiveCount = (SInt32)outputSize - negativeCount;
            vDSP_vclr(output, 1, outputSize);
            cblas_scopy(positiveCount, audioChannel, 1, &output[negativeCount], 1);
            
            return;
        }
        else if ((readOffset + (SInt32)outputSize) >= self->frameCount) {
            
            SInt32 negativeCount = -readOffset;
            SInt32 positiveCount = (SInt32)self->frameCount;
            vDSP_vclr(output, 1, outputSize);
            cblas_scopy(positiveCount, audioChannel, 1, &output[negativeCount], 1);
            
            return;
        }
    }
    else if (readOffset >= 0 && readOffset < self->frameCount) {
        
        if ((readOffset + (SInt32)outputSize) < self->frameCount) {
            
            cblas_scopy((UInt32)outputSize, &audioChannel[readOffset], 1, output, 1);
            
            return;
        }
        else if ((readOffset + (SInt32)outputSize) >= self->frameCount) {
            
            SInt32 samplesCount = (SInt32)(self->frameCount - readOffset);
            vDSP_vclr(output, 1, outputSize);
            cblas_scopy(samplesCount, &audioChannel[readOffset], 1, output, 1);
            
            return;
        }
    }
    else if (readOffset >= self->frameCount) {
        
        vDSP_vclr(output, 1, outputSize);
        
        return;
    }
}

void AudioObject_readCallbackStereo(void *inRefCon, UInt32 inNumberFrames, Float32 **audioData)
{
    AudioObject *self = (AudioObject *)inRefCon;
    
    for (size_t i = 0; i <= self->channelCount; ++i) {
        
        AudioObject_readSamples(self,
                                (SInt32)self->currentFramePosition,
                                (SInt32)i,
                                audioData[i],
                                inNumberFrames);
    }
    
    self->currentFramePosition += inNumberFrames;
}

void AudioObject_readCallbackMono(void *inRefCon, UInt32 inNumberFrames, Float32 *audioData)
{
    AudioObject *self = (AudioObject *)inRefCon;
    
    AudioObject_readSamples(self,
                            (SInt32)self->currentFramePosition,
                            (SInt32)-1,
                            audioData,
                            inNumberFrames);
    
    self->currentFramePosition += inNumberFrames;
}
