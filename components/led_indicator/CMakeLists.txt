
set(priv_requires
    led_strip
    )

set(requires
    log
    freertos
    )

idf_component_register(SRCS "led_indicator.c" INCLUDE_DIRS "include" REQUIRES ${requires} PRIV_REQUIRES ${priv_requires})