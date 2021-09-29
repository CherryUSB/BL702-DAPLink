set(BSP_COMMON_DIR ${CMAKE_SOURCE_DIR}/bsp/bsp_common)
set(CMSIS_DAP_DIR ${CMAKE_CURRENT_LIST_DIR}/DAP)
set(TARGET_REQUIRED_LIBS usb_stack)
set(TARGET_REQUIRED_PRIVATE_INCLUDE ${CMSIS_DAP_DIR}/Include ${BSP_COMMON_DIR}/usb)
set(TARGET_REQUIRED_SRCS ${BSP_COMMON_DIR}/usb/usb_dc.c daplink_process.c winusb.c
                         ${CMSIS_DAP_DIR}/Source/DAP.c
                         ${CMSIS_DAP_DIR}/Source/JTAG_DP.c
                         ${CMSIS_DAP_DIR}/Source/SW_DP.c
                         ${BSP_COMMON_DIR}/usb/uart_interface.c)

set(mains main.c)
generate_bin()
