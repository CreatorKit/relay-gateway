# Add executable targets
########################
ADD_EXECUTABLE(relay_gateway_appd relay_gateway.c flow_interface.c)

# Add library targets
#####################
FIND_LIBRARY(LIB_FLOWCORE libflowcore.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_FLOWMESSAGING libflowmessaging.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_FLOWDEVMGMT libawa.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_CONFIG libconfig.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_GNUTLS libgnutls.so PATHS ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(relay_gateway_appd ${LIB_FLOWDEVMGMT} ${LIB_GNUTLS} ${LIB_FLOWCORE} ${LIB_FLOWMESSAGING} ${LIB_CONFIG})

# Add install targets
######################
INSTALL(TARGETS relay_gateway_appd RUNTIME DESTINATION bin)
