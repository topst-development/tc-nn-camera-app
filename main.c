/*
 * Copyright Telechips Inc.
 *
 * TCC Version 1.0
 *
 * This source code contains confidential information of Telechips.
 *
 * Any unauthorized use without a written permission of Telechips including not
 * limited to re-distribution in source or binary form is strictly prohibited.
 *
 * This source code is provided "AS IS" and nothing contained in this source code
 * shall constitute any express or implied warranty of any kind, including without
 * limitation, any warranty of merchantability, fitness for a particular purpose
 * or non-infringement of any patent, copyright or other third party intellectual
 * property right.
 * No warranty is made, express or implied, regarding the information's accuracy,
 * completeness, or performance.
 *
 * In no event shall Telechips be liable for any claim, damages or other
 * liability arising from, out of or in connection with this source code or
 * the use in the source code.
 *
 * This source code is provided subject to the terms of a Mutual Non-Disclosure
 * Agreement between Telechips and Company.
 */
#include <time.h>
#include "camera_api.h"
#include "display_api.h"
#include "scaler_api.h"

#define DEFAULT_INPUT_PATH CAMERA_DEV_NAME_2
#define DEFAULT_MEMORY_NAME "overlay"
#define DEFAULT_INPUT_WIDTH (1280)
#define DEFAULT_INPUT_HEIGHT (720)
#define DEFAULT_OUTPUT_WIDTH (800)
#define DEFAULT_OUTPUT_HEIGHT (480)
#define DEFAULT_OUTPUT_POSITION_X (0)
#define DEFAULT_OUTPUT_POSITION_Y (0)
#define DEFAULT_DEBUG_MODE DEBUG_MODE_DISABLE
#define PMAP_SPLIT_NUMBER (2)
#define PMAP_SIZE (1920*1080*4)

typedef enum
{
	DEBUG_MODE_DISABLE = 0,
	DEBUG_MODE_LOG,
	DEBUG_MODE_MAX
}debug_mode_t;
static const struct {
	char *debugModeStr[2];
} opt_str = {
	.debugModeStr = {"off", "log"},
};

typedef struct param_info
{
	char* inputPath;
	char* outputPath;
    char* memName;
    uint32_t inputWidth;
    uint32_t inputHeight;
	uint32_t outputWidth;
	uint32_t outputHeight;
    uint32_t outputPostionX;
	uint32_t outputPostionY;
    debug_mode_t debugMode;
}param_info_t;

static int getReservedMemory(const unsigned char *memName, uint64_t *base, uint64_t *size, uint64_t *desPaddr);
static int deinitMemory(uint64_t *desPaddr, uint32_t outputWidth, uint32_t outputHeight);
static double getCurrentTime(void);
static void printUsage(void);
static void getParserArgs(param_info_t *param, int argc, char** argv);
static int32_t adjustRes(uint32_t *punXres, uint32_t *punYres);

int i = 0;

int main(int argc, char **argv)
{
    uint8_t *vaddr;
    uint64_t paddr;
    param_info_t *param = malloc(sizeof(param_info_t));

    CameraHandle cameraHandle;
    DisplayHandle displayHandle;
    ScalerHandle scalerHandle;

    scaler_size_info_t scalerSrcInfo;
    scaler_size_info_t scalerDesInfo;

    uint64_t reservedMemory;
    uint64_t reservedMemorySize;

    uint64_t desPaddr[2];
    uint8_t desPaddrIdx = 0;

    double bgn, end;

    // Parsing parameters
    getParserArgs(param, argc, argv);

    // Init Handle
    CameraCreate(&cameraHandle);
    DisplayCreate(&displayHandle);
    ScalerCreate(&scalerHandle);

    // Open Devices
    CameraOpenDevice(cameraHandle, param->inputPath);
    DisplayOpenDevice(displayHandle, param->outputPath);
    ScalerOpenDevice(scalerHandle, SCALER_DEV_NAME_0, SCALER_INDEX_0);

    // Set Camera Config
    CameraSetConfig(cameraHandle, param->inputWidth, param->inputHeight);

    // Find Reserved memory area using /proc/reserved_mem
    getReservedMemory(param->memName, &reservedMemory, &reservedMemorySize, &desPaddr);

   	int32_t bAdjust = adjustRes(&param->outputWidth, &param->outputHeight);
	if(bAdjust == -1)
		printf("--- adusting resolution failed\n");


    while (1)
    {
        bgn = getCurrentTime();
        CameraGetBuffer(cameraHandle, &vaddr, &paddr);
        
        scalerSrcInfo.width = param->inputWidth;
        scalerSrcInfo.height = param->inputHeight;
        scalerSrcInfo.format = SCALER_FORMAT_ARGB8888;
        scalerSrcInfo.pmap = paddr;
        scalerDesInfo.width = param->outputWidth;
        scalerDesInfo.height = param->outputHeight;
        scalerDesInfo.format = SCALER_FORMAT_RGB888;
        scalerDesInfo.pmap = desPaddr[desPaddrIdx];

        ScalerResize(scalerHandle, SCALER_INDEX_0, scalerSrcInfo, scalerDesInfo);
        ScalerPoll(scalerHandle, SCALER_INDEX_0);

        DisplayShow(displayHandle, desPaddr[desPaddrIdx],  param->outputPostionX, param->outputPostionY, param->outputWidth, param->outputHeight);

        CameraReleaseBuffer(cameraHandle);

        desPaddrIdx = (desPaddrIdx + 1) % 2;  
        end = getCurrentTime();

        if(param->debugMode == DEBUG_MODE_LOG){
            printf("[DEBUG] FPS:%.3f\n", 1.0/(end-bgn));
        }
    }
    CameraCloseDevice(cameraHandle);
    ScalerCloseDevice(scalerHandle, SCALER_INDEX_0);
    DisplayCloseDevice(displayHandle);

    deinitMemory(&desPaddr, param->outputWidth, param->outputHeight);

    CameraDestroy(cameraHandle);
    DisplayDestroy(displayHandle);
    ScalerDestroy(scalerHandle);

    free(param);
}

static int32_t adjustRes(uint32_t *punXres, uint32_t *punYres) {
	int resfd = -1;

    printf("[Info] reading framebuffer resolution from fb@0\n");
	resfd = open("/proc/device-tree/fb@0/xres", O_RDONLY);
	if(resfd > 0)
	{
		char buf[4];
		uint32_t xRes = 0;
		int bytes_read = read(resfd, buf, sizeof(buf));
		if(bytes_read > 0)
		{
			memcpy(&xRes, buf, 4);
			xRes = __builtin_bswap32(xRes);
		}

		*punXres = (xRes >= *punXres)? *punXres : xRes;
		close(resfd);
	} else {
		return -1;
	}	

	resfd = open("/proc/device-tree/fb@0/yres", O_RDONLY);
	if(resfd > 0)
	{
		char buf[4];
		uint32_t yRes = 0;
		int bytes_read = read(resfd, buf, sizeof(buf));
		if(bytes_read > 0)
		{
			memcpy(&yRes, buf, 4);
			yRes = __builtin_bswap32(yRes);
		}
		*punYres = (yRes >= *punYres)? *punYres : yRes;
		close(resfd);
	} else {
		return -1;
	}	

	printf("####### finall Witdh = %d and Height=%d\n", *punXres, *punYres);
	return 0;
}

static int getReservedMemory(const unsigned char *memName, uint64_t *base, uint64_t *size, uint64_t *desPaddr)
{
    FILE *file = fopen("/proc/reserved_mem", "r");
    char line[100];
    char readname[30];
    uint64_t start, end;
    int ret = -1;

    if (file == NULL)
    {
        perror("Failed open memory device");
        return ret;
    }

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "%llx-%llx %*s %s", &start, &end, readname) == 3)
        {
            if (strcmp(readname, memName) == 0)
            {
                *size = end - start + 1;
                *base = start;
                ret = 0;
                break;
            }
        }
    }
    if(ret == 0){
        for(int i = 0; i < PMAP_SPLIT_NUMBER; i++)
        {
            desPaddr[i] = *base + (PMAP_SIZE * i);
        }
    }
    else{
        perror("Fail to find reserved memory");
    }

    return ret;
}

static int deinitMemory(uint64_t *desPaddr, uint32_t outputWidth, uint32_t outputHeight)
{
    int ret = 0;
    if (munmap(desPaddr[0], outputWidth * outputHeight * 3) == -1)
	{
		perror("Error unmapping desPaddr[0]");
		ret = -1; 
	}

	if (munmap(desPaddr[1], outputWidth * outputHeight * 3) == -1)
	{
		perror("Error unmapping desPaddr[1]");
		ret = -1; 
	}

	return ret;
}

static double getCurrentTime(void)
{
	double CurrentTime;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	CurrentTime = (double)ts.tv_sec + (double)ts.tv_nsec / (1000.0 * 1000.0 * 1000.0);

	return CurrentTime;
}

static void printUsage(void)
{
	printf("------------------------------ Usage ------------------------------\n");
    printf("[Common]Print Usage Message    : -? \n");
    printf("[Common]Input Path             : -p default[%s]\n", DEFAULT_INPUT_PATH);
	printf("[Common]Output Path            : -P default[%s]\n", DISPLAY_DEV_NAME_0);
    printf("[Common]Input Width            : -w default[%d] max: 2560\n", DEFAULT_OUTPUT_WIDTH);
	printf("[Common]Input Height           : -h default[%d]  max: 1440\n", DEFAULT_OUTPUT_HEIGHT);
	printf("[Common]Output Width           : -W default[%d] max: 1920\n", DEFAULT_OUTPUT_WIDTH);
	printf("[Common]Output Height          : -H default[%d]  max: 720\n", DEFAULT_OUTPUT_HEIGHT);
    printf("[Common]Output Position X      : -X default[%d]    max: 1920\n", DEFAULT_OUTPUT_POSITION_X);
	printf("[Common]Output Position Y      : -Y default[%d]    max: 720\n", DEFAULT_OUTPUT_POSITION_Y);
    printf("[Debug] Debug Mode             : -g default[%s]  option: off, log\n", opt_str.debugModeStr[DEFAULT_DEBUG_MODE]);
	printf("------------------------------ Usage ------------------------------\n\n");
}

static void getParserArgs(param_info_t *param, int argc, char** argv)
{
    int opt;
    param->inputPath = DEFAULT_INPUT_PATH;
	param->outputPath = DISPLAY_DEV_NAME_0;
    param->memName = DEFAULT_MEMORY_NAME;
    param->inputWidth = DEFAULT_INPUT_WIDTH;
    param->inputHeight = DEFAULT_INPUT_HEIGHT;
    param->outputWidth = DEFAULT_OUTPUT_WIDTH;
	param->outputHeight = DEFAULT_OUTPUT_HEIGHT;
    param->outputPostionX = DEFAULT_OUTPUT_POSITION_X;
    param->outputPostionY = DEFAULT_OUTPUT_POSITION_Y;
    param->debugMode = DEBUG_MODE_DISABLE;

    while(-1 != (opt = getopt(argc, argv, "p:P:w:h:W:H:X:Y:g:?")))
    {
		switch (opt) {
            case 'p':
                if (strcmp(optarg, "/dev/video0") == 0){
                    param->inputPath = CAMERA_DEV_NAME_0;
                }
                else if (strcmp(optarg, "/dev/video1") == 0){
                    param->inputPath = CAMERA_DEV_NAME_1;
                }
                else if (strcmp(optarg, "/dev/video2") == 0){
                    param->inputPath = CAMERA_DEV_NAME_2;
                }
                else if (strcmp(optarg, "/dev/video3") == 0){
                    param->inputPath = CAMERA_DEV_NAME_3;
                }
                else{
                    printf("The input value is invalid. Keeping input path: %s.\n", param->inputPath);
                }
                break;
            case 'P':
                if (strcmp(optarg, "/dev/overlay") == 0){
                    param->outputPath = DISPLAY_DEV_NAME_0;
                    param->memName = "overlay";
                }
                else if (strcmp(optarg, "/dev/overlay1") == 0){
                    param->outputPath = DISPLAY_DEV_NAME_1;
                    param->memName = "overlay1";
                }
                else if (strcmp(optarg, "/dev/overlay2") == 0){
                    param->outputPath = DISPLAY_DEV_NAME_2;
                    param->memName = "overlay2";
                }
                else{
                    printf("The input value is invalid. The default value, output path: /dev/overlay0, will be applied.\n");
                }
                break;
            case 'w':
                if(atoi(optarg) > 0 && atoi(optarg) <= 2560){
                    param->inputWidth = atoi(optarg);
                }
                else{
                    printf("The input value is invalid. The default value, output width: 1920, will be applied.\n");
                }
                break;
            case 'h':
                if(atoi(optarg) > 0 && atoi(optarg) <= 1440){
                    param->inputHeight = atoi(optarg);
                }
                else{
                    printf("The input value is invalid. The default value, output height: 720, will be applied.\n");
                }
                break;
            case 'W':
                if(atoi(optarg) > 0 && atoi(optarg) <= 1920){
                    param->outputWidth = atoi(optarg);
                }
                else{
                    printf("The input value is invalid. The default value, output width: 1920, will be applied.\n");
                }
                break;
            case 'H':
                if(atoi(optarg) > 0 && atoi(optarg) <= 720){
                    param->outputHeight = atoi(optarg);
                }
                else{
                    printf("The input value is invalid. The default value, output height: 720, will be applied.\n");
                }
                break;
            case 'X':
                param->outputPostionX = atoi(optarg);
                break;
            case 'Y':
                param->outputPostionY = atoi(optarg);
                break;
            case 'g':
				if(strcmp(optarg, "off") == 0)
				{
					param->debugMode = DEBUG_MODE_DISABLE;
				}
				else if(strcmp(optarg, "log") == 0)
				{
					param->debugMode = DEBUG_MODE_LOG;
				}
				else
				{
					printf("The input value is invalid. The default value, debug mode: off, will be applied.\n");
				}
				break;
            case '?':
                printUsage();
				exit(0);
				break;
            default:
				printUsage();
				exit(0);
				break;
        }
    }

    printf("------------------ Parameter Info ------------------\n");
    printf("[Common]Input Path             : %s\n", param->inputPath);
	printf("[Common]Output Path            : %s\n", param->outputPath);
    printf("[Common]Input Width            : %d\n", param->inputWidth);
	printf("[Common]Input Height           : %d\n", param->inputHeight);
	printf("[Common]Output Width           : %d\n", param->outputWidth);
	printf("[Common]Output Height          : %d\n", param->outputHeight);
    printf("[Common]Output Position X      : %d\n", param->outputPostionX);
	printf("[Common]Output Position Y      : %d\n", param->outputPostionY);
    printf("[Debug] Debug Mode             : %s\n", opt_str.debugModeStr[param->debugMode]);
	printf("---------------------- Usage ----------------------\n\n");

}
