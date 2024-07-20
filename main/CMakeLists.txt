set(requires log hardware_driver voice_assistant)

idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES ${requires})
