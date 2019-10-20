#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "c2numpy.h"
#include "utf8.h"

int main(int argc, char **argv)
{
  printf("ENTERED FUNCTION [%s]\n", __func__);
  printf("start\n");

  c2numpy_writer writer;

  c2numpyData_t int8_array;
  c2numpyData_t float_array;
  c2numpyData_t unicode_array;
  c2numpyData_t complex_float_array;

  c2numpy_init(&writer, "testout671", 100);

  if(c2numpy_addcolumn(&writer, "int8", C2NUMPY_INT8, 1) == -1)
  {
    printf("Could not add column");
  }
  if(c2numpy_addcolumn(&writer, "float", C2NUMPY_FLOAT32, 8) == -1)
  {
    printf("Could not add column");
  }
  if(c2numpy_addcolumn(&writer, "unicode", C2NUMPY_UNICODE, 8) == -1)
  {
    printf("Could not add column");
  }
  if(c2numpy_addcolumn(&writer, "fc32", C2NUMPY_COMPLEX64, 2) == -1)
  {
    printf("Could not add column");
  }
  // completely optional: writing will open a file if not explicitly called
  c2numpy_ret ret =c2numpy_open(&writer);

  if(ret == C2NUMPY_FILE_WRITE_MODE)
  {
    printf("row %d by separate calls\n", writer.currentRowInFile);
    printf("\n  col %d\n", writer.currentColumn);
    int8_t int_array[1] = {3};
    c2numpy_set_array_properties(&int8_array, int_array,  sizeof(int_array)/sizeof(int_array[0]), sizeof(int_array[0]), NULL);
    if(c2numpy_write_array(&writer, &int8_array) == ERR_C2NUMPY_SIZE_MISMATCH)
    {
      printf("Could not add column");
    }


    printf("\n  col %d\n", writer.currentColumn);
    float flt_array[3] = {3.2, 2.0, NAN};
    float flt_padding[1] = {NAN};
    c2numpy_set_array_properties(&float_array, flt_array, sizeof(flt_array)/sizeof(flt_array[0]), sizeof(flt_array[0]), flt_padding);
    if(c2numpy_write_array(&writer, &float_array) == ERR_C2NUMPY_SIZE_MISMATCH)
    {
      printf("Could not add column");
    }

    printf("\n  col %d\n", writer.currentColumn);
    char* string = "THREE1";
    size_t codepointNum = 0;
    uint32_t* codepointBuffer = NULL;
    uint32_t* unicodePadding = NULL;
    if(check_count_valid_UTF8(string, &codepointNum) == UTF8_ACCEPT)
    {
      codepointBuffer = (uint32_t*)malloc(codepointNum*sizeof(uint32_t));
      unicodePadding = (uint32_t*)malloc(1*sizeof(uint32_t));
      utf8string_create(codepointBuffer, string);
      utf8string_create(unicodePadding, "a");
      c2numpy_set_array_properties(&unicode_array, codepointBuffer, codepointNum, sizeof(codepointBuffer[0]), unicodePadding);
      c2numpy_write_array(&writer, &unicode_array);
    }

    printf("\n  col %d\n", writer.currentColumn);
    complex float complex_fc32_array[2] = {(1.0 - 4.0 * I), (1.0 + 3.0 * I)};
    c2numpy_set_array_properties(&complex_float_array, complex_fc32_array, sizeof(complex_fc32_array)/sizeof(complex_fc32_array[0]), sizeof(complex_fc32_array[0]), NULL);
    if(c2numpy_write_array(&writer, &complex_float_array) == ERR_C2NUMPY_SIZE_MISMATCH)
    {
      printf("Could not add column");
    }

    printf("close\n");
    c2numpy_close(&writer);
  }
  else
  {
    ;
  }

  printf("end\n");
}