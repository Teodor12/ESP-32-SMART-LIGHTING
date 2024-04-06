set(requires
    log
    hardware_driver
    )

idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES ${requires})
