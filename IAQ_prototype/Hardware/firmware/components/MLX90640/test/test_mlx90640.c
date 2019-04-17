#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mbedtls/md.h"

#include "unity.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "esp_log.h"

#include <MLX90640_API.h>
#include "ref_data.h"

//frame variables
float tr=0;

int ret=0;

float* ptr_frame_temp=NULL;

paramsMLX90640* ptr_obtained_parameters=NULL;

void check_hash(void* ref_data, int ref_length, void* actual_data, int actual_length){

    printf("\nReference data and obtained data length in bytes are %d and %d respectively", ref_length, actual_length);
    uint8_t ref_sha[32]={0};
    uint8_t read_sha[32]={0};
    
    mbedtls_md_context_t ref_ctx;
    mbedtls_md_context_t actual_ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ref_ctx);
    mbedtls_md_init(&actual_ctx);
    //Don't use hmac
    mbedtls_md_setup(&ref_ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_setup(&actual_ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ref_ctx);
    mbedtls_md_starts(&actual_ctx);
    //Length in bytes
    mbedtls_md_update(&ref_ctx, (const unsigned char*) ref_data, ref_length);
    mbedtls_md_update(&actual_ctx, (const unsigned char*) actual_data, actual_length);
    mbedtls_md_finish(&ref_ctx, ref_sha);
    mbedtls_md_finish(&actual_ctx, read_sha);
    mbedtls_md_free(&ref_ctx);
    mbedtls_md_free(&actual_ctx);

    printf("\nReference data SHA-256 Hash:");
    for(int i= 0; i< sizeof(ref_sha); i++){
        printf("%02x", ref_sha[i]);
    }
        
    printf("\nobtained data SHA-256 Hash:");
    for(int i= 0; i< sizeof(read_sha); i++){
        printf("%02x", read_sha[i]);
    }
    printf("\n\n");
    
    //Ref param len should be equal to the read param len
    TEST_ASSERT_EQUAL_MEMORY(ref_sha, read_sha, 32);
}

//These helper functions are located in this unit test file in order to verify the data uniformly via the unity framework
void param_check(paramsMLX90640* ref_params, paramsMLX90640* read_params){
    //Calculate and compare SHA256 hashes of both data

    printf("\nReference parameter size is %d bytes long\n",sizeof(*ref_params));
    printf("\nObtained parameter size is %d bytes long\n",sizeof(*read_params));
    
    printf("\nReference kVdd: %d , Obtained kVdd: %d\n", ref_params->vdd25, read_params->vdd25);
    TEST_ASSERT_EQUAL_INT16(ref_params->kVdd, read_params->kVdd);

    printf("\nReference vdd25: %d , Obtained vdd25: %d\n", ref_params->vdd25, read_params->vdd25);
    TEST_ASSERT_EQUAL_INT16(ref_params->vdd25, read_params->vdd25);    

    //printf("\nReference KvPTAT: %f , Obtained KvPTAT: %f\n", ref_params->KvPTAT, read_params->KvPTAT);  
    //TEST_ASSERT_EQUAL_FLOAT(ref_params->KvPTAT, read_params->KvPTAT);

    //printf("\nReference KtPTAT: %f , Obtained KtPTAT: %f\n", ref_params->KtPTAT, read_params->KtPTAT);
    //TEST_ASSERT_EQUAL_FLOAT(ref_params->KtPTAT, read_params->KtPTAT);

    printf("\nReference vPTAT25: %d , Obtained vPTAT25: %d\n", ref_params->vPTAT25, read_params->vPTAT25);    
    TEST_ASSERT_EQUAL_UINT16(ref_params->vPTAT25, read_params->vPTAT25);

    printf("\nReference alphaPTAT: %f , Obtained alphaPTAT: %f\n", ref_params->alphaPTAT, read_params->alphaPTAT);
    TEST_ASSERT_EQUAL_FLOAT(ref_params->alphaPTAT, read_params->alphaPTAT);

    printf("\nReference gainEE: %d , Obtained gainEE: %d\n", ref_params->gainEE, read_params->gainEE);
    TEST_ASSERT_EQUAL_INT16(ref_params->gainEE, read_params->gainEE);

    printf("\nReference tgc: %f , Obtained tgc: %f\n", ref_params->tgc, read_params->tgc);
    TEST_ASSERT_EQUAL_FLOAT(ref_params->tgc, read_params->tgc);

    printf("\nReference cpKv: %f , Obtained cpKv: %f\n", ref_params->cpKv, read_params->cpKv);
    TEST_ASSERT_EQUAL_FLOAT(ref_params->cpKv, read_params->cpKv);

    printf("\nReference cpKta: %f , Obtained cpKta: %f\n", ref_params->cpKta, read_params->cpKta);    
    TEST_ASSERT_EQUAL_FLOAT(ref_params->cpKta, read_params->cpKta);

    printf("\nReference resolutionEE: %d , Obtained resolutionEE: %d\n", ref_params->resolutionEE, read_params->resolutionEE);      
    TEST_ASSERT_EQUAL_UINT8(ref_params->resolutionEE, read_params->resolutionEE);
    
    printf("\nReference calibrationModeEE: %d , Obtained calibrationModeEE: %d\n", ref_params->calibrationModeEE, read_params->calibrationModeEE); 
    TEST_ASSERT_EQUAL_UINT8(ref_params->calibrationModeEE, read_params->calibrationModeEE);

    printf("\nReference KsTa: %f , Obtained KsTa: %f\n", ref_params->KsTa, read_params->KsTa);     
    TEST_ASSERT_EQUAL_FLOAT(ref_params->KsTa, read_params->KsTa);
    
    check_hash((void*)ref_params->ksTo, sizeof(ref_params->ksTo), (void*) read_params->ksTo,  sizeof(read_params->ksTo));
    check_hash((void*)ref_params->ct, sizeof(ref_params->ct), (void*) read_params->ct,  sizeof(read_params->ct));
    check_hash((void*)ref_params->alpha, sizeof(ref_params->alpha), (void*) read_params->alpha,  sizeof(read_params->alpha));
    check_hash((void*)ref_params->offset, sizeof(ref_params->offset), (void*) read_params->offset,  sizeof(read_params->offset));
    check_hash((void*)ref_params->kta, sizeof(ref_params->kta), (void*) read_params->kta,  sizeof(read_params->kta));
    check_hash((void*)ref_params->kv, sizeof(ref_params->kv), (void*) read_params->kv,  sizeof(read_params->kv));
    check_hash((void*)ref_params->cpAlpha, sizeof(ref_params->cpAlpha), (void*) read_params->cpAlpha,  sizeof(read_params->cpAlpha));
    check_hash((void*)ref_params->cpOffset, sizeof(ref_params->cpOffset), (void*) read_params->cpOffset, sizeof(read_params->cpOffset));
    check_hash((void*)ref_params->ilChessC, sizeof(ref_params->ilChessC), (void*) read_params->ilChessC, sizeof(read_params->ilChessC));
    check_hash((void*)ref_params->brokenPixels, sizeof(ref_params->brokenPixels), (void*) read_params->brokenPixels, sizeof(read_params->brokenPixels));
    check_hash((void*)ref_params->outlierPixels, sizeof(ref_params->outlierPixels), (void*) read_params->outlierPixels, sizeof(read_params->outlierPixels));
}

TEST_CASE("test_mlx90640", "test_mlx90640]"){
    //Calls to these functions are to be wrapped in unit test functions to ensure they are being called okay. This is separated from the checking of the results in thefunctions above
    ptr_obtained_parameters=(paramsMLX90640*)malloc(sizeof(paramsMLX90640));
    memset(ptr_obtained_parameters,0,sizeof(paramsMLX90640));
    
    ptr_frame_temp=(float*)malloc(sizeof(ptr_frame_0_data));
    memset(ptr_frame_temp,0,sizeof(ptr_frame_0_data));
    printf("Bytes %d\n",sizeof(ptr_frame_0_data));
    //1. Load the sample EEPROM data into the MCU memory allocated for it...Done on the stack

    //*heap_caps_check_integrity_all(0);
    //2. Call the MLX90640_ExtractParameters(pEEPROM, pParams) function
    ret=MLX90640_ExtractParameters(ptr_eeprom_data, ptr_obtained_parameters);
    TEST_ASSERT_EQUAL_INT(0, ret);

    //*heap_caps_check_integrity_all(0);
    //3. Download the calculated parameters and compare to the ones in the sample data file.
    param_check(&reference_parameters, ptr_obtained_parameters);

    //*heap_caps_check_integrity_all(0);
    //4. Load the sample frame 0 data into the MCU memory allocated for the frame data and call
    tr = MLX90640_GetTa(ptr_frame_0_data, ptr_obtained_parameters) - 8;
    printf("tr value is %f",tr);    
    //*heap_caps_check_integrity_all(0);
    MLX90640_CalculateTo(ptr_frame_0_data, ptr_obtained_parameters, 1.0, tr, ptr_frame_temp);

    //*heap_caps_check_integrity_all(0);
    //5. Download the calculated frame temperatures and compare with the ones in the sample data file
    check_hash((void*)ptr_frame_0_To, sizeof(ptr_frame_0_To), (void*)ptr_frame_temp, sizeof(ptr_frame_temp));

    //6. Load the sample frame 1 data into the MCU memory allocated for the frame data and call
    tr = MLX90640_GetTa(ptr_frame_1_data, ptr_obtained_parameters) - 8; 
    MLX90640_CalculateTo(ptr_frame_1_data, ptr_obtained_parameters, 1.0, tr, ptr_frame_temp);

    //7. Download the calculated frame temperatures and compare with the ones in the sample data file
    //signature_check((void*)ptr_frame_1_To, (void*)ptr_frame_temp);
}
