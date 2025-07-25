idf_component_register(
    SRCS "led_manager.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES "ble_manager" 
    REQUIRES
        led_strip
        json
)
