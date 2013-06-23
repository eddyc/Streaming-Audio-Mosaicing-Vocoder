    //
    //  Matrix.c
    //  ConcatenationRewrite
    //
    //  Created by Edward Costello on 28/02/2013.
    //  Copyright (c) 2013 Edward Costello. All rights reserved.
    //

#import "Matrix.h"
#import "hdf5.h"
#import "MathematicalFunctions.h"
#import <Accelerate/Accelerate.h>

Matrix32 *Matrix32_new(size_t rowCount, size_t columnCount)
{
    
    Matrix32 *self = calloc(1, sizeof(Matrix32));
    self->rowCount = rowCount;
    self->columnCount = columnCount;
    self->elementCount = self->rowCount * self->columnCount;
    self->capacity = nextPowerOfTwo(self->elementCount);
    self->data = calloc(self->capacity, sizeof(Float32));
    self->tempData = calloc(self->capacity, sizeof(Float32));
    self->multiplierData = calloc(self->capacity, sizeof(Float32));
    
    return self;
}

void Matrix32_delete(Matrix32 *self)
{
    free(self->data);
    free(self->tempData);
    free(self->multiplierData);
    free(self);
    self = NULL;
}

void Matrix32_setElementCount(Matrix32 *input, size_t rowCount, size_t columnCount)
{
    size_t elementCount = rowCount * columnCount;
    
    if (elementCount > input->capacity) {
        
        printf("Error new element count larger than matrix capacity, exiting\n");
        exit(-1);
    }
    
    input->rowCount = rowCount;
    input->columnCount = columnCount;
    input->elementCount = elementCount;
}


void Matrix32_saveAsHDF(Matrix32 *self, char *path, char *dataSet)
{
    hid_t fileID, dataSetID, dataSpaceID;
    hsize_t dimensions[2];
    herr_t status;
    
    fileID = H5Fcreate(path,
                       H5F_ACC_TRUNC,
                       H5P_DEFAULT,
                       H5P_DEFAULT);
    
    dimensions[0] = self->rowCount;
    dimensions[1] = self->columnCount;
    
    dataSpaceID = H5Screate_simple(2, dimensions, NULL);
    
    
    dataSetID = H5Dcreate(fileID,
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
    status = H5Fclose(fileID);
}

void Matrix32_copyRows(Matrix32 *source, size_t sourceStartRow, size_t size, Matrix32 *destination, size_t destinationStartRow)
{
    if ((size + destinationStartRow) > destination->rowCount
        ||
        source->columnCount != destination->columnCount) {
        
        printf("Matrix32_copyRows\nIllegal Matrix dimenstions, exiting");
        exit(-1);
    }
    
    size_t elementCount = size * source->columnCount;
    
    cblas_scopy((SInt32)elementCount, Matrix_getRow(source, sourceStartRow), 1, Matrix_getRow(destination, destinationStartRow), 1);
}

void Matrix32_print(Matrix32 *self)
{
    printf("Matrix32_print:\n\nrows = %zd, columns = %zd\n\n", self->rowCount, self->columnCount);
    
    for (size_t i = 0; i < self->rowCount; ++i) {
        
        for (size_t j = 0; j < self->columnCount; ++j) {
            
            printf("[%.5g]\t", self->data[i * self->columnCount + j]);
        }
        printf("\n");
    }
    
    printf("\n\nEnd\n\n");
}


void Matrix32_square(Matrix32 *input, Matrix32 *output)
{
    if (output->elementCount >= input->elementCount) {
        
        vDSP_vsq(input->data, 1, output->data, 1, input->elementCount);
    }
    else {
        
        printf("Matrix_square output element count smaller than input element count\nExiting\n");
        exit(-1);
    }
}

void Matrix32_multiply(Matrix32 *inputA,
                       Boolean transposeA,
                       Matrix32 *inputB,
                       Boolean transposeB,
                       Matrix32 *result)
{
    size_t ARowCount;
    size_t AColumnCount;
    size_t BRowCount;
    size_t BColumnCount;
    
    if (transposeA == false) {
        
        ARowCount = inputA->rowCount;
        AColumnCount = inputA->columnCount;
    }
    else {
        
        ARowCount = inputA->columnCount;
        AColumnCount = inputA->rowCount;
    }
    
    if (transposeB == false) {
        
        BRowCount = inputB->rowCount;
        BColumnCount = inputB->columnCount;
    }
    else {
        
        BRowCount = inputB->columnCount;
        BColumnCount = inputB->rowCount;
    }
    
    if (AColumnCount != BRowCount
        ||
        ARowCount != result->rowCount
        ||
        BColumnCount != result->columnCount
        ) {
        
        printf("Matrix dimensions incompatible for multiplication\nExiting\n");
        exit(-1);
    }
    
    if (transposeA == false && transposeB == false) {
        
        cblas_sgemm(CblasRowMajor,
                    CblasNoTrans,
                    CblasNoTrans,
                    (SInt32)ARowCount,
                    (SInt32)BColumnCount,
                    (SInt32)AColumnCount,
                    1.0,
                    inputA->data,
                    (SInt32)AColumnCount,
                    inputB->data,
                    (SInt32)BColumnCount,
                    0.,
                    result->data,
                    (SInt32)result->columnCount);
        return;
    }
    else if (transposeA == true && transposeB == false) {
        
        cblas_sgemm(CblasRowMajor,
                    CblasTrans,
                    CblasNoTrans,
                    (SInt32)ARowCount,
                    (SInt32)BColumnCount,
                    (SInt32)AColumnCount,
                    1.0,
                    inputA->data,
                    (SInt32)ARowCount,
                    inputB->data,
                    (SInt32)BColumnCount,
                    0.,
                    result->data,
                    (SInt32)result->columnCount);
        return;
    }
    else if (transposeA == false && transposeB == true) {
        
        cblas_sgemm(CblasRowMajor,
                    CblasNoTrans,
                    CblasTrans,
                    (SInt32)ARowCount,
                    (SInt32)BColumnCount,
                    (SInt32)AColumnCount,
                    1.0,
                    inputA->data,
                    (SInt32)AColumnCount,
                    inputB->data,
                    (SInt32)BRowCount,
                    0.,
                    result->data,
                    (SInt32)result->columnCount);
        return;
    }
    else if (transposeA == true && transposeB == true) {
        
        cblas_sgemm(CblasRowMajor,
                    CblasTrans,
                    CblasTrans,
                    (SInt32)ARowCount,
                    (SInt32)BColumnCount,
                    (SInt32)AColumnCount,
                    1.0,
                    inputA->data,
                    (SInt32)ARowCount,
                    inputB->data,
                    (SInt32)BRowCount,
                    0.,
                    result->data,
                    (SInt32)result->columnCount);
        return;
    }
}

void Matrix32_squareRoot(Matrix32 *input, Matrix32 *output)
{
    const int elementCount = (int)input->elementCount;
    vvsqrtf(output->data, input->data, &elementCount);
}

void Matrix32_fill(Matrix32 *input, Float32 scalar)
{
    vDSP_vfill(&scalar, input->data, 1, input->elementCount);
}

void Matrix32_elementWiseDivide(Matrix32 *inputA,
                                Matrix32 *inputB,
                                Matrix32 *result)
{
    if (inputA->rowCount != inputB->rowCount
        ||
        inputA->columnCount != inputB->columnCount
        ||
        inputA->elementCount != inputB->elementCount) {
        
        printf("Matrix32_elementWiseDivide, Matrices not compatible for division, exiting\n");
        exit(-1);
    }
    vDSP_vdiv(inputA->data, 1, inputB->data, 1, result->data, 1, inputA->elementCount);
}

void Matrix32_addRows(Matrix32 *input,
                      Matrix32 *result)
{
    Float32 one = 1;
    vDSP_vfill(&one, input->multiplierData, 1, input->columnCount);
    cblas_scopy((SInt32)input->elementCount, input->data, 1, input->tempData, 1);
    vDSP_mmul(input->multiplierData, 1, input->tempData, 1, result->data, 1, 1, input->columnCount, input->rowCount);
    result->rowCount = 1;
}


void Matrix32_addColumns(Matrix32 *input,
                         Matrix32 *result)
{
    
    Float32 one = 1;
    vDSP_vfill(&one, input->multiplierData, 1, input->elementCount);
    vDSP_mtrans(input->data, 1, input->tempData, 1, input->columnCount, input->rowCount);
    vDSP_mmul(input->multiplierData, 1, input->tempData, 1, result->data, 1, 1, input->rowCount, input->columnCount);
    
    result->columnCount = 1;
}

void Matrix32_fillRowWithScalar(Matrix32 *input,
                                size_t row,
                                Float32 scalar)
{
    if (row >= input->rowCount) {
        
        printf("Matrix32_fillRowWithScalar specified row is greater than input row count\nExiting\n");
        exit(-1);
    }
    vDSP_vfill(&scalar, Matrix_getRow(input, row), 1, input->columnCount);
}

void Matrix32_fillColumnWithScalar(Matrix32 *input,
                                   size_t column,
                                   Float32 scalar)
{
    if (column >= input->columnCount) {
        
        printf("Matrix32_fillColumnWithScalar specified column is greater than input column count\nExiting\n");
        exit(-1);
    }
    vDSP_vfill(&scalar, &Matrix_getRow(input, 0)[column], input->columnCount, input->rowCount);
}

MatrixSizeType *MatrixSizeType_new(size_t rowCount, size_t columnCount)
{
    
    MatrixSizeType *self = calloc(1, sizeof(MatrixSizeType));
    self->rowCount = rowCount;
    self->columnCount = columnCount;
    self->elementCount = self->rowCount * self->columnCount;
    self->capacity = nextPowerOfTwo(self->elementCount);
    self->data = calloc(self->capacity, sizeof(size_t));
    self->tempData = calloc(self->capacity, sizeof(size_t));
    self->multiplierData = calloc(self->capacity, sizeof(size_t));
    
    return self;
}

void MatrixSizeType_delete(MatrixSizeType *self)
{
    free(self->data);
    free(self->tempData);
    free(self->multiplierData);
    free(self);
    self = NULL;
}
