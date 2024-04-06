
set(requires
    log
    esp-sr
    )

set(priv_requires
    hardware_driver
    )

idf_component_register(SRCS "voice_assistant.c" INCLUDE_DIRS "include" REQUIRES ${requires} PRIV_REQUIRES ${priv_requires})