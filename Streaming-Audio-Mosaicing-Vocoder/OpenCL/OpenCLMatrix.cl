typedef struct MatrixFloatPrivate {
    
    __private float *data;
    size_t rowCount;
    size_t columnCount;
    
} MatrixFloatPrivate;

typedef struct MatrixFloatGlobal {
    
    __global float *data;
    size_t rowCount;
    size_t columnCount;
    
} MatrixFloatGlobal;

typedef struct MatrixFloatLocal {
    
    __local float *data;
    size_t rowCount;
    size_t columnCount;
    
} MatrixFloatLocal;


MatrixFloatGlobal MatrixFloatGlobal_new(__global float *data,
                                        size_t rowCount,
                                        size_t columnCount)
{
    MatrixFloatGlobal self;
    self.data = data;
    self.rowCount = rowCount;
    self.columnCount = columnCount;
    
    return self;
}

MatrixFloatLocal MatrixFloatLocal_new(__local float *data,
                                      size_t rowCount,
                                      size_t columnCount)
{
    MatrixFloatLocal self;
    self.data = data;
    self.rowCount = rowCount;
    self.columnCount = columnCount;
    
    return self;
}


MatrixFloatPrivate MatrixFloatPrivate_new(__private float data[],
                                          size_t rowCount,
                                          size_t columnCount)
{
    MatrixFloatPrivate self;
    self.data = data;
    self.rowCount = rowCount;
    self.columnCount = columnCount;
    
    return self;
}

#define Matrix_getElement(self, row, column) (self.data[(row) * self.columnCount + (column)])
#define Matrix_getRow(self,row) (&self.data[(row) * self.columnCount])

void MatrixFloatGlobal_print(MatrixFloatGlobal self)
{
    printf("MatrixFloatGlobal print\nRows = %d, Columns = %d\n", self.rowCount, self.columnCount);
    
    for (size_t i = 0; i < self.rowCount; ++i) {
        
        for (size_t j = 0; j < self.columnCount; ++j) {
            
            printf("[%g]\t", Matrix_getElement(self,i,j));
        }
        
        printf("\n");
    }
}

void MatrixFloatLocal_print(MatrixFloatLocal self)
{
    printf("MatrixFloatLocal print\nRows = %d, Columns = %d\n", self.rowCount, self.columnCount);
    
    for (size_t i = 0; i < self.rowCount; ++i) {
        
        for (size_t j = 0; j < self.columnCount; ++j) {
            
            printf("[%.0f]\t", Matrix_getElement(self,i,j));
        }
        
        printf("\n");
    }
}

void MatrixFloatPrivate_print(MatrixFloatPrivate self)
{
    printf("MatrixFloatPrivate print\nRows = %d, Columns = %d\n", self.rowCount, self.columnCount);
    
    for (size_t i = 0; i < self.rowCount; ++i) {
        
        for (size_t j = 0; j < self.columnCount; ++j) {
            
            printf("[%.0f]\t", Matrix_getElement(self,i,j));
        }
        
        printf("\n");
    }
}


