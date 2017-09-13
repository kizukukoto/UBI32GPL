ifeq ($(CONFIG_USER_WL_RALINK), y)
        @echo =================== CONFIG_USER_WL_RALINK ===================
.PHONY: Ralink
Ralink:
	$(MAKE) -C Ralink
else
        @echo =================== CONFIG_USER_WL_ATH ===================
.PHONY: Atheros
Atheros:
	$(MAKE) -C Atheros
endif
