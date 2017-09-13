#include $(PROJECTS_PATH)/.config

COMPILER_FLAGS += -DEN

ifeq ($(CONFIG_M17N_TW),y)
COMPILER_FLAGS+=-DTW
endif

ifeq ($(CONFIG_M17N_CN),y)
COMPILER_FLAGS+=-DCN
endif

ifeq ($(CONFIG_M17N_JA),y)
COMPILER_FLAGS+=-DJA
endif

#ifeq ($(CONFIG_M17N_DE),y)
COMPILER_FLAGS+=-DDE
#endif

#ifeq ($(CONFIG_M17N_ES),y)
COMPILER_FLAGS+=-DES
#endif

#ifeq ($(CONFIG_M17N_FR),y)
COMPILER_FLAGS+=-DFR
#endif

#ifeq ($(CONFIG_M17N_IT),y)
COMPILER_FLAGS+=-DIT
#endif

ifeq ($(CONFIG_M17N_KO),y)
COMPILER_FLAGS+=-DKO
endif

CONFIG_MODEL_NAME=DIR-652

#ifeq ($(CONFIG_LP),y)
DEFAULT_LANG = EN #`grep lingual $(PROJECTS_PATH)/include/nvram.default |sed -e 's/lingual=//'`
LP_VER=1.0 #`sed -n 's/\#define LP_VER \"\([0-9]\{4\}\)\"/\1/p' lp_version.h`
LP_JS=lingualMTD
CFLAGS += -DCONFIG_MODEL_NAME="\"$(CONFIG_MODEL_NAME)\"" -DCONFIG_LP
#endif

js_converter: *.c lp_version.h
	gcc $(COMPILER_FLAGS) $(CFLAGS) -o $@ $^

install_m17n_%:
#ifeq ($(CONFIG_LP),y)
#Generate $(LP_JS)
	 $(if $(findstring $(subst install_m17n_,,$@),EN TW), \
	./js_converter -l $(subst install_m17n_,,$@) -c $(STRING_PATH)STRINGS_ZH.txt -t ./$(LP_JS), \
	./js_converter -l $(subst install_m17n_,,$@) -c $(STRING_PATH)STRINGS_$(subst install_m17n_,,$@).txt -t ./$(LP_JS))
	./release_LP.sh . $(CONFIG_MODEL_NAME) $(LP_VER) $(subst install_m17n_,,$@) $(LP_JS)
#touch $(FS_PATH)/www/$(LP_JS).js is used to prevent httpd return error with there is no $(LP_JS).js in /www/
	if [ $(DEFAULT_LANG) = $(subst install_m17n_,,$@) ];\
	then \
	 echo "document.write(\"<script language=JavaScript type=text\/javascript src=$(LP_JS).js><\/script>\");" >> ./$(LP_JS);\
	  cp -f ./$(LP_JS) $(FS_PATH)/www/lingual_$(subst install_m17n_,,$@).js; \
	  touch $(FS_PATH)/www/$(LP_JS).js; \
	fi
	rm -f ./$(LP_JS)
#endif

ifeq ($(CONFIG_M17N),y)
	$(if $(findstring $(subst install_m17n_,,$@),EN TW), \
$(PROJECTS_PATH)/tools/m17n/js_converter -l $(subst install_m17n_,,$@) -c $(STRING_PATH)STRINGS_ZH.txt -t $(FS_PATH)/www, \
$(PROJECTS_PATH)/tools/m17n/js_converter -l $(subst install_m17n_,,$@) -c $(STRING_PATH)STRINGS_$(subst install_m17n_,,$@).txt -t $(FS_PATH)/www)
endif

clean:
	rm -f js_converter
	rm -f *.js
