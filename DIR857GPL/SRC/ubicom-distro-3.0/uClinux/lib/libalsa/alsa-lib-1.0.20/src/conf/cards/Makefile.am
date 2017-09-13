alsaconfigdir = @ALSA_CONFIG_DIR@
alsadir = $(alsaconfigdir)/cards
cfg_files = aliases.conf \
	Ubi32-CS4350.conf \
	Ubi32-CS4384.conf \
	Ubi32-Generic-C.conf \
	Ubi32-Generic.conf 

if BUILD_ALISP
cfg_files += aliases.alisp
endif

alsa_DATA = $(cfg_files)

if BUILD_ALISP
SI7018dir = $(alsaconfigdir)/cards/SI7018
SI7018_files = \
	SI7018/sndoc-mixer.alisp \
	SI7018/sndop-mixer.alisp
SI7018_DATA = $(SI7018_files)
else
SI7018_files=
endif

EXTRA_DIST = \
	$(cfg_files)
