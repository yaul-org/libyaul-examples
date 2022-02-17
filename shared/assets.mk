define macro-builtin-asset-rule
$(call macro-convert-build-path,$(addsuffix .o,$1)): $1
	@printf -- "$(V_BEGIN_CYAN)$(strip $1)$(V_END)\n"
	$(ECHO)$(YAUL_INSTALL_ROOT)/bin/bin2o $1 $2 $$@

SH_SRCS+= $(addsuffix .o,$1)
endef

$(foreach BUILTIN_ASSET,$(BUILTIN_ASSETS), \
	$(eval $(call macro-builtin-asset-rule,\
		$(call macro-word-split,$(BUILTIN_ASSET),1), \
		$(call macro-word-split,$(BUILTIN_ASSET),2))))

undefine macro-builtin-asset-rule
