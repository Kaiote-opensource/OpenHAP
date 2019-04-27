#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/*Naive implementation of this as floatig point values have numerical limits*/
esp_err_t floatMaxValue(float myArray[], size_t size, float* maxValuePtr);

esp_err_t intMaxValue(int myArray[], size_t size, int* maxValuePtr);

/*Naive implementation of this as floatig point values have numerical limits*/
esp_err_t floatMinValue(float myArray[], size_t size, float* minValuePtr);

esp_err_t intMinValue(int myArray[], size_t size, int* minValuePtr);

esp_err_t floatSumValue(float myArray[], size_t size, int* sumValuePtr);

esp_err_t intSumValue(int myArray[], size_t size, int* sumValuePtr);

esp_err_t meanValue(float myArray[], size_t size, float* meanValuePtr);

esp_err_t variance(float myArray[], size_t size, float* varianceValuePtr);

esp_err_t stddev(float myArray[], size_t size, float* stddevPtr);

#endif