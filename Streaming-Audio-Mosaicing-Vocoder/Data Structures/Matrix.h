//
//  Matrix.h
//  ConcatenationRewrite
//
//  Created by Edward Costello on 28/02/2013.
//  Copyright (c) 2013 Edward Costello. All rights reserved.
//

#import <MacTypes.h>
#import <stdlib.h>

#define Matrix_getRow(self,row) (&(self)->data[(row) * (self)->columnCount])

#ifdef __cplusplus
extern "C"
{
#endif
    
    /*!
     @class Matrix32
     @abstract A pseudoclass for storing matrices used for MATLAB style matrix arithmetic.
     @var rowCount;
     The number of rows in the matrix.
     @var columnCount;
     The number of columns in the matrix.
     @var elementCount;
     The number of elements in the matrix, rows * columns.
     @var capacity;
     The overall capacity of the matrix, matrices are stored as the nearest power of 2 above elementCount.
     @var data;
     A pointer to the data contained in the matrix.
     @var tempData;
     A pointer to temporary data used to do some in place arithmetic operations.
     */
    typedef struct Matrix32
    {
        size_t rowCount;
        size_t columnCount;
        size_t elementCount;
        size_t capacity;
        Float32 *data;
        Float32 *tempData;
        Float32 *multiplierData;
        
    } Matrix32;
    /*!
     @functiongroup Construct/Destruct
     */
    
    /*!
     Contruct a Matrix32 pseudoclass
     @param rowCount
     The number of rows in the matrix.
     @param columnCount
     The number of columns in the matrix.
     */
    Matrix32 *Matrix32_new(size_t rowCount, size_t columnCount);
    /*!
     Destroy a Matrix32 pseudoclass
     @param self
     Pointer to the matrix to be destroyed.
     */
    void Matrix32_delete(Matrix32 *self);
    
    /*!
     @functiongroup Saving
     */
    
    /*!
     Save the contents of a matrix to a HDF data file.
     @param self
     Pointer to the matrix to be saved.
     @param path
     Location of where to save the file.
     @param dataSet
     Data-set label in HDF file.
     */
    void Matrix32_saveAsHDF(Matrix32 *self, char *path, char *dataSet);
    void Matrix32_copyRows(Matrix32 *source, size_t sourceStartRow, size_t size, Matrix32 *destination, size_t destinationStartRow);
    void Matrix32_print(Matrix32 *input);
    void Matrix32_setElementCount(Matrix32 *input, size_t rowCount, size_t columnCount);
    
    void Matrix32_square(Matrix32 *input, Matrix32 *output);
    void Matrix32_squareRoot(Matrix32 *input, Matrix32 *output);
    
    void Matrix32_fill(Matrix32 *input, Float32 scalar);
    
    void Matrix32_multiply(Matrix32 *inputA,
                           Boolean transposeA,
                           Matrix32 *inputB,
                           Boolean transposeB,
                           Matrix32 *result);
    
    void Matrix32_elementWiseDivide(Matrix32 *inputA,
                                    Matrix32 *inputB,
                                    Matrix32 *result);
    
    void Matrix32_addRows(Matrix32 *input,
                          Matrix32 *result);
    
    
    void Matrix32_addColumns(Matrix32 *input,
                             Matrix32 *result);
    
    void Matrix32_fillRowWithScalar(Matrix32 *input,
                                    size_t row,
                                    Float32 scalar);
    
    void Matrix32_fillColumnWithScalar(Matrix32 *input,
                                       size_t column,
                                       Float32 scalar);

    
    typedef struct MatrixSizeType
    {
        size_t rowCount;
        size_t columnCount;
        size_t elementCount;
        size_t capacity;
        size_t *data;
        size_t *tempData;
        size_t *multiplierData;
        
    } MatrixSizeType;
    
    MatrixSizeType *MatrixSizeType_new(size_t rowCount, size_t columnCount);

    void MatrixSizeType_delete(MatrixSizeType *self);
    
    
    
#ifdef __cplusplus
}
#endif