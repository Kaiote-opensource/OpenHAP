#include <math.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>

static char* TAG = "statistics";

esp_err_t floatMaxValue(const float myArray[], const size_t size, float* maxValuePtr, const bool exitInfNan, size_t* computeSize)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || maxValuePtr == NULL || computeSize == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    float maxValue = myArray[0];
    size_t validElements = 0;
    
    for (size_t i = 1; i < size; ++i)
    {
        if(!(isnan(myArray[i])) && !(isinf(myArray[i])))
        {
            if ( isgreater(myArray[i], maxValue) )
            {
                maxValue = myArray[i];
            }
        }
        else
        {
            ESP_LOGE(TAG, "Float array contains Nan or inf element at index %zu", i);
            if(exitInfNan)
            {
                /*Exit on the first inf or Nan element if exitInfNan is any other value aside from 0*/
                *computeSize = validElements;
                maxValuePtr = NULL;
                return ESP_FAIL;
            }
        }
    }
    *maxValuePtr = maxValue;
    *computeSize = validElements;
    ESP_LOGD(TAG, "Obtained max value as %f over the range of %zu valid elements", maxValue, validElements);
    return ESP_OK;
}

esp_err_t intMaxValue(const int myArray[], const size_t size, int* maxValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || maxValuePtr == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int maxValue = myArray[0];

    for (size_t i = 1; i < size; ++i)
    {
        if ( myArray[i] > maxValue )
        {
            maxValue = myArray[i];
        }
    }

    *maxValuePtr = maxValue;
    ESP_LOGD(TAG, "Obtained min as %d", maxValue);
    return ESP_OK;
}

esp_err_t floatMinValue(const float myArray[], const size_t size, float* minValuePtr, const bool exitInfNan, size_t* computeSize)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || minValuePtr == NULL || computeSize == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    
    float minValue = myArray[0];
    size_t validElements = 0;

    for (size_t i = 1; i < size; ++i)
    {
        if(!(isnan(myArray[i])) && !(isinf(myArray[i])))
        {
            if ( isless(myArray[i], minValue) )
            {
                minValue = myArray[i];
                validElements +=1;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Float array contains Nan or inf element at index %zu", i);
            if(exitInfNan)
            {
                /*Exit on the first inf or Nan element if exitInfNan is any other value aside from 0*/
                *computeSize = validElements;
                minValuePtr = NULL;
                return ESP_FAIL;
            }
        }
    }
    *minValuePtr = minValue;
    *computeSize = validElements;
    ESP_LOGD(TAG, "Obtained min as %f over the range of %zu valid elements", minValue, validElements);
    return ESP_OK;
}

esp_err_t intMinValue(const int myArray[], const size_t size, int* minValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || minValuePtr == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    
    int minValue = myArray[0];

    for (size_t i = 1; i < size; ++i)
    {
        if ( myArray[i] < minValue )
        {
            minValue = myArray[i];
        }
    }
    *minValuePtr = minValue;
    ESP_LOGD(TAG, "Obtained min as %d", minValue);
    return ESP_OK;
}

esp_err_t floatSumValue(const float myArray[], const size_t size, float* sumValuePtr, const bool exitInfNan, size_t* computeSize)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || sumValuePtr == NULL || computeSize == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    float sum = 0.0;
    size_t validElements = 0;

    for (size_t i = 1; i < size; i++)
    {        
        if(!(isnan(myArray[i])) && !(isinf(myArray[i])))
        {
            sum +=  myArray[i];
            validElements += 1;
        }
        else
        {
            ESP_LOGE(TAG, "Float array contains Nan or inf element at index %zu, ", i);
            if(exitInfNan)
            {
                /*Exit on the first inf or Nan element if exitInfNan is any other value aside from 0*/
                *computeSize = validElements;
                sumValuePtr = NULL;
                return ESP_FAIL;
            }
        }
    }
    *sumValuePtr = sum;
    *computeSize = validElements;
    ESP_LOGD(TAG, "Obtained sum as %f over the range of %zu valid elements", sum, validElements);
    return ESP_OK;
}

esp_err_t intSumValue(const int myArray[], const size_t size, int* sumValuePtr)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || sumValuePtr == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int sum = 0;

    for (size_t i = 1; i < size; i++)
    {
        sum +=  myArray[i];
    }
    *sumValuePtr = sum;
    ESP_LOGD(TAG, "Obtained sum as %d", sum);
    return ESP_OK;
}

esp_err_t meanValue(const float myArray[], const size_t size, float* meanValuePtr, const bool exitInfNan, size_t* computeSize)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL || size == 0 || meanValuePtr == NULL || computeSize == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    float sum = 0.0;
    size_t validElements = 0;

    /*Calculate sum of values, taking note of the boolean controlling ignoring of inf and Nan elements*/
    if(floatSumValue(myArray, size, &sum, exitInfNan, &validElements) != ESP_OK)
    {
        *computeSize = validElements;
        meanValuePtr = NULL;
        return ESP_FAIL;
    }

    *meanValuePtr = sum/validElements;
    *computeSize = validElements;
    ESP_LOGD(TAG, "Obtained mean as %f over the range of %zu valid elements", *meanValuePtr, validElements);
    return ESP_OK;
}

esp_err_t variance(const float myArray[], const size_t size, float* varianceValuePtr, const bool exitInfNan, size_t* computeSize)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL ||  size == 0 || varianceValuePtr == NULL || computeSize == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    float sum = 0.0;
    float avg = 0.0;
    size_t validElements = 0;

    if(meanValue(myArray, size, &avg, exitInfNan, &validElements) != ESP_OK)
    {
        *computeSize = validElements;
        varianceValuePtr = NULL;
        return ESP_FAIL;        
    }

    for (size_t i = 0; i < size; i++)
    {
        if(!(isnan(myArray[i])) && !(isinf(myArray[i])))
        {
            sum += pow((myArray[i] - avg), 2);
        }
    }
    *varianceValuePtr = sum / validElements;
    *computeSize = validElements;
    ESP_LOGD(TAG, "Obtained variance as %f", *varianceValuePtr);

    return ESP_OK;
}

esp_err_t stddev(const float myArray[], const size_t size, float* stddevPtr, const bool exitInfNan, size_t* computeSize)
{
    ESP_LOGD(TAG, "ENTERED FUNCTION [%s]", __func__);
    if(myArray == NULL ||  size == 0 || stddevPtr == NULL || computeSize == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    float var = 0.0;

    if(variance(myArray, size, &var, exitInfNan, computeSize) != ESP_OK)
    {
        return ESP_FAIL;
    }

    *stddevPtr = (float)sqrt(var);
    return ESP_OK;
}
