set(requires log freertos )

set(priv_requires led_strip)

idf_component_register(SRCS "led_indicator.c" INCLUDE_DIRS "include"  REQUIRES ${requires}PRIV_REQUIRES ${priv_requires})