#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

esp_err_t floatMaxValue(const float myArray[], const const size_t size, float* maxValuePtr, const int exitInfNan, size_t* computeSize);

esp_err_t intMaxValue(const int myArray[], const size_t size, int* maxValuePtr);

esp_err_t floatMinValue(const float myArray[], const size_t size, float* minValuePtr, const int exitInfNan, size_t* computeSize);

esp_err_t intMinValue(const int myArray[], const size_t size, int* minValuePtr);

esp_err_t floatSumValue(const float myArray[], const size_t size, float* sumValuePtr, const int exitInfNan, size_t* computeSize);

esp_err_t intSumValue(const int myArray[], const size_t size, int* sumValuePtr);

esp_err_t meanValue(const float myArray[], const size_t size, float* meanValuePtr, const int exitInfNan, size_t* computeSize);

esp_err_t variance(const float myArray[], const size_t size, float* varianceValuePtr, const int exitInfNan, size_t* computeSize);

esp_err_t stddev(const float myArray[], const size_t size, float* stddevPtr, const int exitInfNan, size_t* computeSize);

#endif