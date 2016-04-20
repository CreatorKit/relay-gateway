# Add executable targets
########################
ADD_EXECUTABLE(relay_control_appd relay_control.c flow_interface.c)
#ADD_EXECUTABLE(flow_temperature_gateway_appd interface.c)
# Add library targets
#####################
FIND_LIBRARY(LIB_FLOWCORE libflowcore.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_FLOWMESSAGING libflowmessaging.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_FLOWDEVMGMT libawa.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_CONFIG libconfig.so PATHS ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_GNUTLS libgnutls.so.30 PATHS ${STAGING_DIR}/usr/lib)
TARGET_LINK_LIBRARIES(relay_control_appd ${LIB_FLOWDEVMGMT} ${LIB_GNUTLS} ${LIB_FLOWCORE} ${LIB_FLOWMESSAGING} ${LIB_CONFIG})

# Add install targets
######################
INSTALL(TARGETS relay_control_appd RUNTIME DESTINATION bin)
