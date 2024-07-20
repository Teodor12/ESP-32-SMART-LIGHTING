set(requires log driver)

idf_component_register( SRCS "bsp.c" INCLUDE_DIRS "include" REQUIRES ${requires} )