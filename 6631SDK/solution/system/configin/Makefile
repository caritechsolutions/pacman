protect_config:
	@cp -f $(GXSRC_PATH)"/projects/"$(BR2_FAMILY_NAME)"/"$(BR2_PROJ_NAME)/xconfig .config
	@$(BUILD_DIR)/config/mconf Config.in.protect
	@mv .config  $(GXSRC_PATH)"/projects/"$(BR2_FAMILY_NAME)"/"$(BR2_PROJ_NAME)/xconfig
