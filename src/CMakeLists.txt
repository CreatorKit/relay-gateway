# Add executable targets
########################
ADD_EXECUTABLE(relay_gateway_appd relay_gateway.c)
# Add library targets
#####################
FIND_LIBRARY(LIB_AWA_STATIC libawa_static.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_CONFIG libconfig.so ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(relay_gateway_appd ${LIB_CONFIG} ${LIB_AWA_STATIC})

# Add install targets
######################
INSTALL(TARGETS relay_gateway_appd RUNTIME DESTINATION bin)
