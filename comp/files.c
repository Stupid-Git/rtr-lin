
#include "file_structs.h"

#include "files.h"

#include <stdio.h>
#include <string.h>

fact_config_t fact_config;
my_config_t my_config;
uint8_t group_data[SFM_GROUP_SIZE];
uint8_t regist_data[SFM_REGIST_SIZE_32]; //SFM_REGIST_START
uint8_t temp_data[SFM_TEMP_SIZE];

uint8_t CertFile_WS_1[4096];
uint8_t CertFile_WS_2[4096];
uint8_t CertFile_WS_3[4096];
uint8_t CertFile_WS_4[4096];
uint8_t CertFile_WS_5[4096];
uint8_t CertFile_WS_6[4096];
uint8_t CertFile_WS_7[4096];
uint8_t CertFile_WS_8[4096];

uint8_t CertFile_USER_1[4096];
uint8_t CertFile_USER_2[4096];
uint8_t CertFile_USER_3[4096];
uint8_t CertFile_USER_4[4096];
uint8_t CertFile_USER_5[4096];
uint8_t CertFile_USER_6[4096];
uint8_t CertFile_USER_7[4096];
uint8_t CertFile_USER_8[4096];

uint8_t log_data[SFM_LOG_SIZE];
uint8_t td_log_data[SFM_TD_LOG_SIZE];


//EDF uint8_t xml_buffer[4096]; //  __attribute__((section(".xram")));
///証明書作業用バッファ
uint8_t CertFile_WS[4096];
///証明書作業用バッファ
uint8_t CertFile_USER[4096];
///グループバッファ
//Not used uint8_t Group_Buffer[1024];


void file_struct_info(void)
{

	printf("fact_config     size = %ld\n", sizeof(fact_config));
	printf("my_config       size = %ld\n", sizeof(my_config));
	printf("group_data      size = %ld\n", sizeof(group_data));
	printf("regist_data     size = %ld\n", sizeof(regist_data));
	printf("temp_data       size = %ld\n", sizeof(temp_data));
	printf("CertFile_WS_1   size = %ld\n", sizeof(CertFile_WS_1));
	printf("CertFile_USER_1 size = %ld\n", sizeof(CertFile_USER_1));

	printf("log_data        size = %ld\n", sizeof(log_data));
	printf("td_log_data     size = %ld\n", sizeof(td_log_data));
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int file_make_if_not_exist__my_config();
int file_make_if_not_exist__my_config()
{
    FILE* file;
    // open the file in rb mode
    file = fopen("config_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        //return 1;

        file = fopen("config_data.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            return 1;
        }

        memset(&my_config, 0, sizeof(my_config));
        //strcpy(my_config.device.Name, "FRED");

        fwrite((char*)&my_config, sizeof(my_config), 1, file );

        fclose( file);
    }

    return 0;
}

int file_store__my_config();
int file_store__my_config()
{
    // open the file in rb mode
    FILE* file = fopen("config_data.bin", "wb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fwrite(&my_config, sizeof(my_config), 1, file);

    // close the file
    fclose(file);

    return 0;
}


int file_load__my_config();
int file_load__my_config()
{
    // open the file in rb mode
    FILE* file = fopen("config_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fread(&my_config, sizeof(my_config), 1, file);

    // Process the read data (e.g., print or manipulate)
    printf("Device Name = %s\n", my_config.device.Name);
    // close the file
    fclose(file);

    return 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int file_make_if_not_exist__my_config_b();
int file_make_if_not_exist__my_config_b()
{
    FILE* file;
    // open the file in rb mode
    file = fopen("config_b_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        //return 1;

        file = fopen("config_b_data.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            return 1;
        }

        memset(&my_config, 0, sizeof(my_config));
        //strcpy(my_config.device.Name, "FRED");

        fwrite((char*)&my_config, sizeof(my_config), 1, file );

        fclose( file);
    }

    return 0;
}

int file_store__my_config_b();
int file_store__my_config_b()
{
    // open the file in rb mode
    FILE* file = fopen("config_b_data.bin", "wb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fwrite(&my_config, sizeof(my_config), 1, file);

    // close the file
    fclose(file);

    return 0;
}


int file_load__my_config_b();
int file_load__my_config_b()
{
    // open the file in rb mode
    FILE* file = fopen("config_b_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fread(&my_config, sizeof(my_config), 1, file);

    // Process the read data (e.g., print or manipulate)
    printf("Device Name = %s\n", my_config.device.Name);
    // close the file
    fclose(file);

    return 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int file_make_if_not_exist__log();
int file_make_if_not_exist__log()
{
	FILE* file;
    // open the file in rb mode
    file = fopen("log_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        //return 1;

        file = fopen("log_data.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            return 1;
        }

        memset(log_data, 0xFF, sizeof(log_data));

        fwrite((char*)&log_data, sizeof(log_data), 1, file );

        fclose( file);
    }

    return 0;
}

int file_store__log();
int file_store__log()
{
    // open the file in rb mode
    FILE* file = fopen("log_data.bin", "wb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fwrite(&log_data, sizeof(log_data), 1, file);

    // close the file
    fclose(file);

    return 0;
}

int file_load__log();
int file_load__log()
{
    // open the file in rb mode
    FILE* file = fopen("log_data.bin", "rb");

    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fread(&log_data, sizeof(log_data), 1, file);

    // close the file
    fclose(file);

    return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int file_make_if_not_exist__registry();
int file_make_if_not_exist__registry()
{
    FILE* file;
    // open the file in rb mode
    file = fopen("regist_data.bin", "rb");
    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        //return 1;
        file = fopen("regist_data.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            return 1;
        }
        memset(regist_data, 0xFF, sizeof(regist_data));
        fwrite((char*)&regist_data, sizeof(regist_data), 1, file );
        fclose( file);
    }
    return 0;
}

int file_store__registry();
int file_store__registry()
{
    // open the file in rb mode
    FILE* file = fopen("regist_data.bin", "wb");
    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    fwrite(&regist_data, sizeof(regist_data), 1, file);
    // close the file
    fclose(file);

    return 0;
}

int file_load__registry();
int file_load__registry()
{
    // open the file in rb mode
    FILE* file = fopen("regist_data.bin", "rb");
    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    fread(&regist_data, sizeof(regist_data), 1, file);
    // close the file
    fclose(file);
    return 0;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int file_make_if_not_exist__group();
int file_make_if_not_exist__group()
{
    FILE* file;
    // open the file in rb mode
    file = fopen("group_data.bin", "rb");
    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        //return 1;
        file = fopen("group_data.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            return 1;
        }
        memset(group_data, 0xFF, sizeof(group_data));
        fwrite((char*)&group_data, sizeof(group_data), 1, file );
        fclose( file);
    }
    return 0;
}

int file_store__group();
int file_store__group()
{
    // open the file in rb mode
    FILE* file = fopen("group_data.bin", "wb");
    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    fwrite(&group_data, sizeof(group_data), 1, file);
    // close the file
    fclose(file);

    return 0;
}

int file_load__group();
int file_load__group()
{
    // open the file in rb mode
    FILE* file = fopen("group_data.bin", "rb");
    // check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    fread(&group_data, sizeof(group_data), 1, file);
    // close the file
    fclose(file);
    return 0;
}
//=============================================================================
//
//=============================================================================
#define INIT_ALL 1
void my_config_file_dummy(void);
void my_config_file_dummy(void)
{
    file_make_if_not_exist__my_config_b();
    file_make_if_not_exist__my_config();
	file_load__my_config();

#if INIT_ALL
    memset(&my_config, 0, sizeof(my_config));
//    fact_config.SerialNumber = 0x3F580123;
    fact_config.SerialNumber = 0x5F580123;
    void init_factory_default(uint8_t type, uint8_t mode);
    init_factory_default(0,0);

    file_store__my_config();
    file_store__my_config_b();
#endif
}

void log_file_dummy(void);
void log_file_dummy(void)
{
    file_make_if_not_exist__log();
    file_load__log();
    file_store__log();
}

void registry_file_dummy(void);
void registry_file_dummy(void)
{
    file_make_if_not_exist__registry();
    file_load__registry();

#if 0 // erase for debugging
    memset(&regist_data[0], 0xff, sizeof(regist_data));
    file_store__registry();
#endif

#if INIT_ALL
    void init_regist_file_default(void);
    init_regist_file_default();

    memset(&regist_data[0], 0xff, sizeof(regist_data)); // erase for debugging

    file_store__registry();
#endif
}

void group_file_dummy(void);
void group_file_dummy(void)
{
    file_make_if_not_exist__group();
    file_load__group();
#if INIT_ALL
    void init_group_file_default(void);
    init_group_file_default();

    file_store__group();
#endif
}

