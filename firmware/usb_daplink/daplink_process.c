#include "usbd_core.h"
#include "DAP_config.h"
#include "DAP.h"

static volatile uint16_t USB_RequestIndexI; // Request  Index In
static volatile uint16_t USB_RequestIndexO; // Request  Index Out
static volatile uint16_t USB_RequestCountI; // Request  Count In
static volatile uint16_t USB_RequestCountO; // Request  Count Out
static volatile uint8_t USB_RequestIdle;    // Request  Idle  Flag

static volatile uint16_t USB_ResponseIndexI; // Response Index In
static volatile uint16_t USB_ResponseIndexO; // Response Index Out
static volatile uint16_t USB_ResponseCountI; // Response Count In
static volatile uint16_t USB_ResponseCountO; // Response Count Out
static volatile uint8_t USB_ResponseIdle;    // Response Idle  Flag

static uint8_t USB_Request[DAP_PACKET_COUNT][DAP_PACKET_SIZE] __attribute__((section(".bss.USB_IO")));  // Request  Buffer
static uint8_t USB_Response[DAP_PACKET_COUNT][DAP_PACKET_SIZE] __attribute__((section(".bss.USB_IO"))); // Response Buffer
static uint16_t USB_RespSize[DAP_PACKET_COUNT];                                                         // Response Size

void usb_dap_recv_callback(uint8_t ep)
{
    if (USB_RequestIdle) {
        return;
    }

    uint32_t n = 0;

    usbd_ep_read(ep, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE, &n);
    if (n != 0U) {
        if (USB_Request[USB_RequestIndexI][0] == ID_DAP_TransferAbort) {
            DAP_TransferAbort = 1U;
        } else {
#if 0
            MSG("Packet Recv, Idx = %d, Type = 0x%02x\r\n", USB_RequestCountI, USB_Request[USB_RequestIndexI][0]);
#endif
            USB_RequestIndexI++;
            if (USB_RequestIndexI == DAP_PACKET_COUNT) {
                USB_RequestIndexI = 0U;
            }
            USB_RequestCountI++;
        }
        usbd_ep_read(ep, NULL, 0, NULL);
    }
    // Start reception of next request packet
    if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) == DAP_PACKET_COUNT) {
        USB_RequestIdle = 1U;
    }
}

void usb_dap_send_callback(uint8_t ep)
{
    if (USB_ResponseCountI != USB_ResponseCountO) {
        // Load data from response buffer to be sent back
#if 0
        MSG("Packet Send, Idx = %d, Type = 0x%02x\r\n", USB_ResponseCountO, USB_Request[USB_ResponseIndexO][0]);
#endif
        usbd_ep_write(ep, USB_Response[USB_ResponseIndexO], USB_RespSize[USB_ResponseIndexO], NULL);
        USB_ResponseIndexO++;
        if (USB_ResponseIndexO == DAP_PACKET_COUNT) {
            USB_ResponseIndexO = 0U;
        }
        USB_ResponseCountO++;
    } else {
        USB_ResponseIdle = 1U;
    }
}

void usb_handle(void)
{
    uint32_t n;

    while (USB_RequestCountI != USB_RequestCountO) {
        // Handle Queue Commands
        // n = USB_RequestIndexO;
        // while (USB_Request[n][0] == ID_DAP_QueueCommands) {
        //     USB_Request[n][0] = ID_DAP_ExecuteCommands;
        //     n++;
        //     if (n == DAP_PACKET_COUNT) {
        //         n = 0U;
        //     }
        //     if (n == USB_RequestIndexI) {
        //         flags = osThreadFlagsWait(0x81U, osFlagsWaitAny, osWaitForever);
        //         if (flags & 0x80U) {
        //             break;
        //         }
        //     }
        // }
        if (USB_Request[USB_RequestIndexO][0] == ID_DAP_QueueCommands) {
            // Try to find next DAP_ExecuteCommands
            for (n = (USB_RequestIndexO + 1) % DAP_PACKET_COUNT; n != USB_RequestIndexI; n = (n + 1) % DAP_PACKET_COUNT) {
                if (USB_Request[n][0] != ID_DAP_QueueCommands) {
                    break;
                }
            }
            if (n == USB_RequestIndexI) {
                continue;
            }
            for (uint32_t t = USB_RequestIndexO; t != n; t = (t + 1) % DAP_PACKET_COUNT) {
                USB_Request[t][0] = ID_DAP_ExecuteCommands;
            }
        }

        // Execute DAP Command (process request and prepare response)
        USB_RespSize[USB_ResponseIndexI] =
            (uint16_t)DAP_ExecuteCommand(USB_Request[USB_RequestIndexO], USB_Response[USB_ResponseIndexI]);

        // Update Request Index and Count
        USB_RequestIndexO++;
        if (USB_RequestIndexO == DAP_PACKET_COUNT) {
            USB_RequestIndexO = 0U;
        }
        USB_RequestCountO++;

        if (USB_RequestIdle) {
            if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT) {
                USB_RequestIdle = 0U;
                // USBD_EndpointRead(0U, USB_ENDPOINT_OUT(1U), USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE);
            }
        }

        // Update Response Index and Count
        USB_ResponseIndexI++;
        if (USB_ResponseIndexI == DAP_PACKET_COUNT) {
            USB_ResponseIndexI = 0U;
        }
        USB_ResponseCountI++;

        // if (USB_ResponseIdle) {
        //     if (USB_ResponseCountI != USB_ResponseCountO) {
        //     // Load data from response buffer to be sent back
        //         n = USB_ResponseIndexO++;
        //         if (USB_ResponseIndexO == DAP_PACKET_COUNT) {
        //             USB_ResponseIndexO = 0U;
        //         }
        //         USB_ResponseCountO++;
        //         USB_ResponseIdle = 0U;
        //         USBD_EndpointWrite(0U, USB_ENDPOINT_IN(1U), USB_Response[n], USB_RespSize[n]);
        //     }
        // }
    }
}

void gpio_init(void)
{
    gpio_set_mode(LED_CONNECTED, GPIO_OUTPUT_MODE);
    gpio_set_mode(LED_RUNNING, GPIO_OUTPUT_MODE);
    gpio_write(LED_CONNECTED, 1U);
    gpio_write(LED_RUNNING, 1U);

}
