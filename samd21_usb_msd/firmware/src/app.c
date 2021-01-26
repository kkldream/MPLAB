#include <string.h>
#include "app.h"
APP_DATA appData;

void APP_Initialize ( void )
{
    appData.state = APP_STATE_BUS_ENABLE;
    appData.deviceIsConnected = false;
    appData.bt2_status = false;
    appData.bt3_status = false;
    appData.boot_times = 0;
    printf("\n\r----------APP_Initialize----------\n\r");
}

USB_HOST_EVENT_RESPONSE APP_USBHostEventHandler(USB_HOST_EVENT event, void * eventData, uintptr_t context) {
    switch (event) {
        case USB_HOST_EVENT_DEVICE_REJECTED_INSUFFICIENT_POWER:
            printf("USB_HOST_EVENT_DEVICE_REJECTED_INSUFFICIENT_POWER\n\r");
            break;
        case USB_HOST_EVENT_DEVICE_UNSUPPORTED:
            printf("USB_HOST_EVENT_DEVICE_UNSUPPORTED\n\r");
            break;
        case USB_HOST_EVENT_HUB_TIER_LEVEL_EXCEEDED:
            printf("USB_HOST_EVENT_HUB_TIER_LEVEL_EXCEEDED\n\r");
            break;
        case USB_HOST_EVENT_PORT_OVERCURRENT_DETECTED:
            printf("USB_HOST_EVENT_PORT_OVERCURRENT_DETECTED\n\r");
            break;
        default:
            printf("USB_HOST_EVENT_ONKNOWN\n\r");
            break;
    }
    return (USB_HOST_EVENT_RESPONSE_NONE);
}

void APP_SYSFSEventHandler(SYS_FS_EVENT event, void * eventData, uintptr_t context) {
    switch (event) {
        case SYS_FS_EVENT_MOUNT:
            printf("SYS_FS_EVENT_MOUNT\n\r");
            appData.deviceIsConnected = true;
            LED3_On();
            break;
        case SYS_FS_EVENT_UNMOUNT:
            printf("SYS_FS_EVENT_UNMOUNT\n\r");
            appData.deviceIsConnected = false;
            appData.state = APP_STATE_WAIT_FOR_DEVICE_ATTACH;
            LED3_Off();
            break;
        case SYS_FS_EVENT_ERROR:
            printf("SYS_FS_EVENT_ERROR\n\r");
            break;
        default:
            printf("SYS_FS_EVENT_ONKNOWN\n\r");
            break;
    }
}

void APP_Tasks(void) {
    switch (appData.state) {
        case APP_STATE_BUS_ENABLE:
            
            /* Set the event handler and enable the bus */
            SYS_FS_EventHandlerSet((void *) APP_SYSFSEventHandler, (uintptr_t) NULL);
            USB_HOST_EventHandlerSet(APP_USBHostEventHandler, 0);
            USB_HOST_BusEnable(USB_HOST_BUS_ALL);
            appData.state = APP_STATE_WAIT_FOR_BUS_ENABLE_COMPLETE;
            printf("Event Handler Initialize\n\r");
            break;

        case APP_STATE_WAIT_FOR_BUS_ENABLE_COMPLETE:
            
            if (USB_HOST_BusIsEnabled(USB_HOST_BUS_ALL)) {
                appData.state = APP_STATE_WAIT_FOR_DEVICE_ATTACH;
                printf("USB Host Bus Is Enabled\n\r");
            }
            break;

        case APP_STATE_WAIT_FOR_DEVICE_ATTACH:

            if (appData.deviceIsConnected) {
                appData.state = APP_STATE_DEVICE_CONNECTED;
                printf("Device Is Connected\n\r");
            }

            break;

        case APP_STATE_DEVICE_CONNECTED:

//            appData.state = APP_STATE_OPEN_FILE;
            appData.state = APP_STATE_SERIAL_EVENT;
            break;
            
        case APP_STATE_SERIAL_EVENT:
        {
            LED1_On();
            char file_name[100];
            scanf("%s", file_name);
            printf("file_name = %s\n\r", file_name);
            char file_path[100];
            sprintf(file_path, "/mnt/myDrive1/%s", file_name);
            printf("file_path = %s\n\r", file_path);
            
            appData.fileHandle = SYS_FS_FileOpen(file_path, (SYS_FS_FILE_OPEN_WRITE));
            if (appData.fileHandle == SYS_FS_HANDLE_INVALID) {
                appData.state = APP_STATE_ERROR;
                printf("File Open Fail\n\r");
                break;
            } else {
                printf("File Open Success\n\r");
            }
            
            LED2_On();
            char writeBuffer[100];
            scanf("%s", writeBuffer);
            printf("Write:\n\r%s\n\r", writeBuffer);
            if (SYS_FS_FileWrite(appData.fileHandle, writeBuffer, strlen(writeBuffer)) == -1) {
                appData.state = APP_STATE_ERROR;
                printf("File Write Fail\n\r");
                break;
            } else {
                appData.state = APP_STATE_CLOSE_FILE;
                printf("File Write Success\n\r");
            }
            LED1_Off();
            LED2_Off();
        }
            break;

        case APP_STATE_OPEN_FILE:

            appData.fileHandle = SYS_FS_FileOpen("/mnt/myDrive1/file.txt", (SYS_FS_FILE_OPEN_APPEND_PLUS));
            if (appData.fileHandle == SYS_FS_HANDLE_INVALID) {
                appData.state = APP_STATE_ERROR;
                printf("File Open Fail\n\r");
            } else {
                appData.state = APP_STATE_BUTTON_EVENT;
                printf("File Open Success\n\r");
            }
            break;
                    
        case APP_STATE_BUTTON_EVENT:
        {
            if (BT2_Get()) appData.bt2_status = false;
            else if (!appData.bt2_status) {
                appData.bt2_status = true;
                LED1_On();
                appData.state = APP_STATE_WRITE_TO_FILE;
            }
            if (BT3_Get()) appData.bt3_status = false;
            else if (!appData.bt3_status) {
                appData.bt3_status = true;
                LED2_On();
                appData.state = APP_STATE_READ_TO_FILE;
            }
        }
            break;

        case APP_STATE_WRITE_TO_FILE:
        {
            char writeBuffer[100];
            sprintf(writeBuffer, "Times = %d\n", (int) appData.boot_times++);
            if (SYS_FS_FileWrite(appData.fileHandle, writeBuffer, strlen(writeBuffer)) == -1) {
                appData.state = APP_STATE_ERROR;
                printf("File Write Fail\n\r");
            } else {
                appData.state = APP_STATE_CLOSE_FILE;
                printf("File Write Success\n\r");
            }
            LED1_Off();
        }
            break;
        case APP_STATE_READ_TO_FILE:
        {
            int32_t file_size = SYS_FS_FileSize(appData.fileHandle);
            char readBuffer[file_size];
//            if (SYS_FS_FileStringGet(appData.fileHandle, readBuffer, file_size) == -1) {
            if (SYS_FS_FileRead(appData.fileHandle, readBuffer, file_size) == -1) {
                appData.state = APP_STATE_ERROR;
                printf("File Read Fail\n\r");
            } else {
                appData.state = APP_STATE_BUTTON_EVENT;
//                printf("Read Size = %d:\n\r%s\n\rFile Read Success\n\r", (int) file_size, readBuffer);
                printf("File Read Success (Size = %d)\n\r", (int) file_size);
            }
            LED2_Off();
        }
            break;

        case APP_STATE_CLOSE_FILE:
            
            SYS_FS_FileClose(appData.fileHandle);
//            appData.state = APP_STATE_OPEN_FILE;
            appData.state = APP_STATE_SERIAL_EVENT;
            printf("File Close Success\n\r");
            break;

        case APP_STATE_ERROR:

            SYS_FS_FileClose(appData.fileHandle);
            if (SYS_FS_Unmount("/mnt/myDrive1") != 0) {
                /* The disk could not be un mounted. Try un mounting again untill success. */
                appData.state = APP_STATE_ERROR;
            } else {
                /* UnMount was successful. Wait for device attach */
                appData.state = APP_STATE_WAIT_FOR_DEVICE_ATTACH;
                appData.deviceIsConnected = false;
            }
            break;

        default:
            break;

    }
}