set(COMPONENT_SRCS "lightbox.c")
set(COMPONENT_PRIV_REQUIRES "ble_manager" "led_manager")

idf_component_register(
    SRCS "${COMPONENT_SRCS}"
    INCLUDE_DIRS "."
    PRIV_REQUIRES "${COMPONENT_PRIV_REQUIRES}"
    REQUIRES
        nvs_flash
)
