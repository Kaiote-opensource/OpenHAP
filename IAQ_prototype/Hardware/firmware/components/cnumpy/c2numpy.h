// Copyright 2016 Jim Pivarski
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef C2NUMPY
#define C2NUMPY

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "endianext.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

//TODO: Add operations to support subsequent opening of npy files

const char* C2NUMPY_VERSION = "1.3";
//This library does not yet support nesting

    typedef enum {
        ERR_C2NUMPY_START = 127,

        ERR_C2NUMPY_TYPE_MISMATCH = -1,
        ERR_C2NUMPY_SIZE_MISMATCH = -2,
        ERR_C2NUMPY_TRUNCATION = -3,

        ERR_C2NUMPY_END = -128
    } c2numpy_error;
// http://docs.scipy.org/doc/numpy/user/basics.types.html
typedef enum {
                           //Only platform independent datatypes
    C2NUMPY_BOOL = 0,      // Boolean (True or False) stored as a byte
    C2NUMPY_INTP,          // Integer used for indexing (same as C ssize_t; normally either int32 or int64)
    C2NUMPY_INT8,          // Byte (-128 to 127)
    C2NUMPY_INT16,         // Integer (-32768 to 32767)
    C2NUMPY_INT32,         // Integer (-2147483648 to 2147483647)
    C2NUMPY_INT64,         // Integer (-9223372036854775808 to 9223372036854775807)
    C2NUMPY_UINT8,         // Unsigned integer (0 to 255)
    C2NUMPY_UINT16,        // Unsigned integer (0 to 65535)
    C2NUMPY_UINT32,        // Unsigned integer (0 to 4294967295)
    C2NUMPY_UINT64,        // Unsigned integer (0 to 18446744073709551615)
    C2NUMPY_FLOAT32,       // Single precision float: sign bit, 8 bits exponent, 23 bits mantissa
    C2NUMPY_FLOAT,         // Shorthand for float64.
    //Packed datatypes, no boundary alignment
    C2NUMPY_FLOAT64,       // Double precision float: sign bit, 11 bits exponent, 52 bits mantissa
    C2NUMPY_COMPLEX64,     // Complex number, represented by two 32-bit floats (real and imaginary components)
    C2NUMPY_COMPLEX,       // Shorthand for complex128.
    C2NUMPY_COMPLEX128,    // Complex number, represented by two 64-bit floats (real and imaginary components)
    C2NUMPY_UNICODE,       // python 3 unicode
    C2NUMPY_STRING,        //NULL terminated string for python 2 compatibility
  
    C2NUMPY_END          = 255   // ensure that c2numpy_type is at least a byte
} c2numpy_type;

typedef struct
{
    struct
    {
        void* pointer;
        size_t len;
    }data;

    void* padding;
    size_t dtype_size;
} c2numpyData_t;

// a Numpy writer object
typedef struct {
    char buffer[16];              // (internal) used for temporary copies in c2numpy_row

    FILE *file;                   // output file handle
    char *outputFilePrefix;       // output file name, not including the rotating number and .npy
    int64_t sizeSeekPosition;     // (internal) keep track of number of rows to modify before closing
    int64_t sizeSeekSize;         // (internal)

    int32_t numColumns;           // number of columns in the record array
    char** columnNames;           // column names
    c2numpy_type *columnTypes;    // column types
    uint32_t* columnWidths;       // column width(Array of number of elements over all columns). Max 4294967296 element per column

    int32_t numRowsPerFile;       // maximum number of rows per file
    int32_t currentColumn;        // current column number
    int32_t currentRowInFile;     // current row number in the current file
    int32_t currentFileNumber;    // current file number
} c2numpy_writer;

char* c2numpy_descr(c2numpy_type type)
{
    printf("ENTERED FUNCTION [%s]", __func__);
    switch (type)
    {
        case C2NUMPY_BOOL:
            return "|b1";
        case C2NUMPY_INTP:
            return isBigEndian()?">i8":"<i8";
        case C2NUMPY_INT8:
            return "|i1";
        case C2NUMPY_INT16:
            return isBigEndian()?">i2":"<i2";
        case C2NUMPY_INT32:
            return isBigEndian()?">i4":"<i4";
        case C2NUMPY_INT64:
            return isBigEndian()?">i8":"<i8";
        case C2NUMPY_UINT8:
            return "|u1";
        case C2NUMPY_UINT16:
            return isBigEndian()?">u2":"<u2";
        case C2NUMPY_UINT32:
            return isBigEndian()?">u4":"<u4";
        case C2NUMPY_UINT64:
            return isBigEndian()?">u8":"<u8";
        case C2NUMPY_FLOAT:
            return isBigEndian()?">f8":"<f8";
        case C2NUMPY_FLOAT32:
            return isBigEndian()?">f4":"<f4";
        case C2NUMPY_FLOAT64:
            return isBigEndian()?">f8":"<f8";
        case C2NUMPY_COMPLEX:
            return isBigEndian()?">c16":"<c16";
        case C2NUMPY_COMPLEX64:
            return isBigEndian()?">c8":"<c8";
        case C2NUMPY_COMPLEX128:
            return isBigEndian()?">c16":"<c16";
        case C2NUMPY_UNICODE:
            return isBigEndian()?">U":"<U";
        case C2NUMPY_STRING:
            return "S";
        default:
            return NULL;
    }

    return NULL;
}

int c2numpy_init(c2numpy_writer *writer, const char *outputFilePrefix, int32_t numRowsPerFile)
{
    printf("ENTERED FUNCTION [%s]\n", __func__);
    writer->file = NULL;
    writer->outputFilePrefix = (char*)malloc(strlen(outputFilePrefix) + 1);
    strcpy(writer->outputFilePrefix, outputFilePrefix);
    writer->sizeSeekPosition = 0;
    writer->sizeSeekSize = 0;

    writer->numColumns = 0;
    writer->columnNames = NULL;
    writer->columnTypes = NULL;
    writer->columnWidths = NULL;

    writer->numRowsPerFile = numRowsPerFile;
    writer->currentColumn = 0;
    writer->currentRowInFile = 0;
    writer->currentFileNumber = 0;
    return 0;
}

int c2numpy_addcolumn(c2numpy_writer *writer, const char *name, c2numpy_type type, uint32_t width)
{
    printf("ENTERED FUNCTION [%s]", __func__);
    writer->numColumns += 1;

    char* newColumnName = (char*)malloc(strlen(name) + 1);
    strcpy(newColumnName, name);

    //Copy heap references to stack allocated pointers to avoid mem leakage
    char** oldColumnNames = writer->columnNames;
    c2numpy_type* oldColumnTypes = writer->columnTypes;
    uint32_t* oldColumnWidths = writer->columnWidths;

    //Create new memory from the heap to store new values
    writer->columnNames = (char**)malloc(writer->numColumns * sizeof(char*));    
    writer->columnTypes = (c2numpy_type*)malloc(writer->numColumns * sizeof(c2numpy_type));
    writer->columnWidths = (uint32_t*)malloc(writer->numColumns * sizeof(uint32_t));

    if(writer->columnNames == NULL || writer->columnTypes == NULL || writer->columnWidths == NULL)
    {
        writer->numColumns -= 1;
        writer->columnNames = oldColumnNames;
        writer->columnTypes = oldColumnTypes;
        writer->columnWidths = oldColumnWidths;
        oldColumnNames=NULL;
        oldColumnTypes=NULL;
        oldColumnWidths=NULL;
        return -1;
    }

    //This loop will not be valid if there was previously no column present
    //Copy back old values fast via memcpy with only one comparison rather than a loop
    // for (int column = 0; column < writer->numColumns - 1; ++column)
    // {
    //     writer->columnNames[column] = oldColumnNames[column];
    //     writer->columnTypes[column] = oldColumnTypes[column];
    //     writer->columnWidths[column] = oldColumnWidths[column];
    // }
    if(((writer->numColumns)-1) != 0)
    {
        memcpy(writer->columnNames, oldColumnNames, sizeof(char*)*((writer->numColumns)-1));
        memcpy(writer->columnTypes, oldColumnTypes, sizeof(c2numpy_type)*((writer->numColumns)-1));
        memcpy(writer->columnWidths, oldColumnWidths, sizeof(uint32_t)*((writer->numColumns)-1));
    }

    //copy the name to avoid pointer aliasing from direct assignent
    //Copy new value
    writer->columnNames[writer->numColumns - 1]  = newColumnName;
    writer->columnTypes[writer->numColumns - 1]  = type;        
    writer->columnWidths[writer->numColumns - 1] = width;

    //Free temporary pointers
    //Assign NULL just for safety of future code added
    if(oldColumnNames != NULL)
    {
        free(oldColumnNames);
        oldColumnNames=NULL;
    }
    if(oldColumnTypes != NULL)
    {
        free(oldColumnTypes);
        oldColumnTypes=NULL;
    }
    if(oldColumnWidths != NULL)
    {
        free(oldColumnWidths);
        oldColumnTypes=NULL;
    }

    return 0;
}

int c2numpy_open(c2numpy_writer *writer)
{
    printf("ENTERED FUNCTION [%s]", __func__);
    char *fileName = (char*)malloc(strlen(writer->outputFilePrefix) + 15);
    sprintf(fileName, "%s%d.npy", writer->outputFilePrefix, writer->currentFileNumber);
    writer->file = fopen(fileName, "wb");

    // FIXME: better initial guess about header size before going in 128 byte increments
    char *header = NULL;
    for (int64_t headerSize = 128;  headerSize <= 4294967295;  headerSize += 128) {
        if (header != NULL)
        {        
            free(header);
        }
        header = (char*)malloc(headerSize + 1);

        char version1 = (headerSize <= 65535);
        uint32_t descrSize;
        if (version1)
        {
            //Descr starts at byte 10, and is less magic, major ver, minor ver and header size
            descrSize = headerSize - 10;
        }
        else
        {
            //Descr starts at byte 12, and is less magic, major ver, minor ver and header size
            descrSize = headerSize - 12;
        }

        header[0] = 0x93;                            // magic
        header[1] = 'N';
        header[2] = 'U';
        header[3] = 'M';
        header[4] = 'P';
        header[5] = 'Y';
        if (version1) 
        {
            header[6] = 0x01;                          // format version 1.0
            header[7] = 0x00;
            const uint16_t descrSize2 = descrSize;
            *(uint16_t*)(header + 8) = descrSize2;   // version 1.0 has a 16-byte descrSize
        }
        else 
        {
            header[6] = 0x02;                          // format version 2.0
            header[7] = 0x00;
            *(uint32_t*)(header + 8) = descrSize;   // version 2.0 has a 32-byte descrSize
        }
        //Get start of descr offset
        int64_t offset = headerSize - descrSize;
        //Use snprintf in order not to overflow the buffer, copy till maximum of header less magic, major ver, minor ver and header size value, including null char
        offset += snprintf((header + offset), headerSize - offset + 1, "{'descr': [");
        //if the buffer is not enough, delete this and allocate more from the heap
        if (offset >= headerSize) continue;

        char* columnTypeFormat =NULL;
        char* shape = NULL;
        for (int column = 0;  column < writer->numColumns;  ++column)
        {
            columnTypeFormat = c2numpy_descr(writer->columnTypes[column]);
            if(writer->columnTypes[column] == C2NUMPY_STRING || writer->columnTypes[column] == C2NUMPY_UNICODE)
            {
                asprintf(&shape, "%u", writer->columnWidths[column]);
            }
            else
            {
                asprintf(&shape, "(%u,)", writer->columnWidths[column]);
            }
            offset += snprintf((header + offset), headerSize - offset + 1, "('%s', '%s', %s)", writer->columnNames[column], columnTypeFormat, shape);
            if (offset >= headerSize)
            {
                columnTypeFormat =NULL;
                free(shape);
                continue;
            }

            if (column < writer->numColumns - 1)
            {
                offset += snprintf((header + offset), headerSize - offset + 1, ", ");
            }
            if (offset >= headerSize)
            {
                columnTypeFormat =NULL;
                free(shape);
                continue;
            }
        }
        if(shape != NULL)
        {
            free(shape);
            shape=NULL;
        }

        offset += snprintf((header + offset), headerSize - offset + 1, "], 'fortran_order': False, 'shape': (");
        if (offset >= headerSize) continue;

        writer->sizeSeekPosition = offset;
        writer->sizeSeekSize = snprintf((header + offset), headerSize - offset + 1, "%d", writer->numRowsPerFile);
        offset += writer->sizeSeekSize;
        if (offset >= headerSize) continue;

        offset += snprintf((header + offset), headerSize - offset + 1, ",), }");
        if (offset >= headerSize) continue;

        while (offset < headerSize)
        {
            if (offset < headerSize - 1)
            {
                header[offset] = ' ';
            }
            else
            {
                header[offset] = '\n';
            }
            offset += 1;
        }
        header[headerSize] = 0;
        fwrite(header, 1, headerSize, writer->file);
        return 0;
    }

    return -1;
}

#define C2NUMPY_CHECK_ITEM {                                                    \
    if (writer->file == NULL) {                                                 \
        int status = c2numpy_open(writer);                                      \
        if (status != 0)                                                        \
            return status;                                                      \
    }                                                                           \
}

#define C2NUMPY_INCREMENT_ITEM {                                                \
    if (writer->currentColumn == 0) {                                           \
        writer->currentRowInFile += 1;                                          \
        if (writer->currentRowInFile == writer->numRowsPerFile) {               \
            fclose(writer->file);                                               \
            writer->file = NULL;                                                \
            writer->currentRowInFile = 0;                                       \
            writer->currentFileNumber += 1;                                     \
        }                                                                       \
    }                                                                                                                                              \
}

c2numpyData_t* c2numpy_set_array_properties(c2numpyData_t* array, void* data, size_t dataLength, size_t dtypeSize, void* paddingElement)
{
    array->data.pointer = data;
    array->data.len = dataLength;
    array->dtype_size = dtypeSize;
    array->padding = paddingElement;
    return array;
}

int c2numpy_write_array(c2numpy_writer *writer, c2numpyData_t* array)
{
    printf("ENTERED FUNCTION [%s]", __func__);
    C2NUMPY_CHECK_ITEM
    uint32_t length = writer->columnWidths[writer->currentColumn];
    // c2numpy_type dataType = writer->columnTypes[writer->currentColumn];
    // if (dataType != C2NUMPY_INTP) 
    // {
    //     return -1;
    // }

    //If the size of data type in header is greater than the passed, array bounds will be inevitably crossed.
    //Take till the passed datatype size and fill the rest with padding.
    //If the opposite occurs, truncate till the size of header dtype multiplied with passed length.
    //If the two datasizes are equal, compare on length only.
    if(array->data.len < length)
    {
        return ERR_C2NUMPY_SIZE_MISMATCH;
    }
    size_t bytesWritten = fwrite((array->data.pointer), (array->dtype_size), length, writer->file);
    if(bytesWritten != length)
    {
        printf("Number of elements written do not match");
        return ERR_C2NUMPY_SIZE_MISMATCH;
    }
    writer->currentColumn = (writer->currentColumn + 1) % writer->numColumns;
    C2NUMPY_INCREMENT_ITEM

    if(array->data.len > length)
    {
        return ERR_C2NUMPY_TRUNCATION;
    }
    return 0;
}

int c2numpy_close(c2numpy_writer *writer)
{
    printf("ENTERED FUNCTION [%s]", __func__);
    if (writer->file != NULL) {
        // we wrote fewer rows than we promised
        if (writer->currentRowInFile < writer->numRowsPerFile)
        {
            // so go back to the part of the header where that was written
            fseek(writer->file, writer->sizeSeekPosition, SEEK_SET);
            // overwrite it with spaces
            for (int i = 0;  i < writer->sizeSeekSize;  ++i)
            {
                fputc(' ', writer->file);
            }
            // now go back and write it again (it MUST be fewer or an equal number of digits)
            fseek(writer->file, writer->sizeSeekPosition, SEEK_SET);
            fprintf(writer->file, "%d", writer->currentRowInFile);
        }
        // now close it
        fclose(writer->file);
    }

    // and clear the malloc'ed memory
    free(writer->outputFilePrefix);
    for (int column = 0;  column < writer->numColumns;  ++column)
    {
        free(writer->columnNames[column]);
    }
    free(writer->columnNames);
    free(writer->columnTypes);
    free(writer->columnWidths);

    return 0;
}

#endif // C2NUMPY