
set(requires
        log
        freertos
        driver
)

idf_component_register(SRCS "pwm_output.c" INCLUDE_DIRS "include" REQUIRES ${requires} PRIV_REQUIRES ${priv_requires})