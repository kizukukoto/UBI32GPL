##############################################################################
#
#	Makefile -- Master makefile for all libraries.
#

.EXPORT_ALL_VARIABLES:

##############################################################################
#
# Include architecture specific build rules.
#

ifndef ROOTDIR
ROOTDIR=..
endif

UCLINUX_BUILD_LIB=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

##############################################################################
#
# always build these dir_
#
dir_1_y  = $(LIBCDIR)
dir_1_n  =
dir_1_   =

ifeq ($(LIBCDIR),libc)
dir_2_y += libm libcrypt_old
dir_2_n +=
dir_2_  +=
endif

ifeq ($(LIBCDIR),uClibc)
dir_2_y += libm
dir_2_n +=
dir_2_  +=
endif

#
# and build these if someone needs them
#
dir_2_$(CONFIG_LIB_LIBFM)        += libfm
dir_2_$(CONFIG_LIB_LIBCRASHLOG)  += libcrashlog

dir_3_$(CONFIG_LIB_UBIDBG)       += libubidbg
dir_3_$(CONFIG_LIB_FLEX)         += flex
dir_3_$(CONFIG_LIB_LIBALSA)      += libalsa
dir_3_$(CONFIG_LIB_FAAD2)     += faad2
dir_3_$(CONFIG_LIB_MAD)          += libmad
dir_3_$(CONFIG_LIB_LIBSSL)       += libssl
dir_3_$(CONFIG_LIB_LIBGMP)       += libgmp
dir_3_$(CONFIG_LIB_LIBG)         += libg
dir_3_$(CONFIG_LIB_LIBPAM)       += libpam
dir_3_$(CONFIG_LIB_LIBPCAP)      += libpcap
dir_3_$(CONFIG_LIB_ZLIB)         += zlib
dir_3_$(CONFIG_LIB_LIBBZ2)       += libbzip2
dir_3_$(CONFIG_LIB_LIBATM)       += libatm
dir_3_$(CONFIG_LIB_LIBNET)       += Libnet
dir_3_$(CONFIG_LIB_LIBNETOLD)    += libnet
dir_3_$(CONFIG_LIB_LIBJPEG)      += libjpeg
dir_3_$(CONFIG_LIB_LIBUPNP)      += libupnp
dir_3_$(CONFIG_LIB_STLPORT)      += STLport
dir_3_$(CONFIG_LIB_EXPAT)        += expat
dir_3_$(CONFIG_LIB_LIBLDAP)      += libldap
dir_3_$(CONFIG_LIB_TINYTCL)      += ../user/tinytcl
dir_3_$(CONFIG_LIB_LIBCCMALLOC)  += libccmalloc
dir_3_$(CONFIG_LIB_ARES)         += libares
dir_3_$(CONFIG_LIB_LIBIDN)       += libidn
dir_3_$(CONFIG_LIB_LIBCRYPT_OLD) += libcrypt_old
dir_3_$(CONFIG_LIB_LIBLZO)       += lzo
dir_3_$(CONFIG_LIB_LIBPCRE_OLD)  += pcre
dir_3_$(CONFIG_LIB_LIBPCRE-7.7)  += pcre-7.7
dir_3_$(CONFIG_LIB_LIBPCRE)      += pcre-7.8
dir_3_$(CONFIG_LIB_LIBLZMA)      += liblzma
dir_3_$(CONFIG_LIB_OSIP2)        += osip2
dir_3_$(CONFIG_LIB_LIBEFENCE)    += libefence
dir_3_$(CONFIG_LIB_TERMCAP)      += termcap
dir_3_$(CONFIG_LIB_LIBSYSFS)     += sysfsutils
dir_3_$(CONFIG_LIB_LIBNL)        += libnl
dir_3_$(CONFIG_USER_TCL_LIBTCL)  += ../user/tcl
dir_3_$(CONFIG_LIB_LIBIBERTY)    += libiberty
dir_3_$(CONFIG_LIB_BFD)          += bfd
dir_3_$(CONFIG_LIB_POPT)         += popt
dir_3_$(CONFIG_LIB_POPT-1.10.4)  += popt-1.10.4
dir_3_$(CONFIG_LIB_LIBJUDY)      += libjudy
dir_3_$(CONFIG_LIB_CLASSPATH)    += classpath
dir_3_$(CONFIG_LIB_LIBFFI)       += libffi
dir_3_$(CONFIG_LIB_BITVECTOR)    += bitvector
dir_3_$(CONFIG_LIB_GIFLIB)       += giflib
dir_3_$(CONFIG_LIB_LIBICONV)     += libiconv
dir_3_$(CONFIG_LIB_PIXMAN)       += pixman
dir_3_$(CONFIG_LIB_LIBEXIF)      += libexif
dir_3_$(CONFIG_LIB_SQLITE3)      += sqlite3
dir_3_$(CONFIG_LIB_DMALLOC)      += dmalloc
dir_3_$(CONFIG_LIB_LIBCONFIG)    += libconfig
dir_3_$(CONFIG_LIB_TAGLIB)       += TagLib

# See Kconfig for the dependencies
dir_4_$(CONFIG_LIB_LIBID3TAG)    += libid3tag # requires zlib(3)
dir_4_$(CONFIG_LIB_LIBXML2)      += libxml2
dir_4_$(CONFIG_LIB_GETTEXT)      += gettext
dir_4_$(CONFIG_LIB_FREETYPE)     += freetype
dir_4_$(CONFIG_LIB_GPGERROR)     += libgpg-error     # requires zlib(3)

dir_4_$(CONFIG_LIB_LIBPNG)       += libpng
dir_3_$(CONFIG_LIB_NFNETLINK)    += libnfnetlink
dir_4_$(CONFIG_LIB_NETFILTER_LOG)+= libnetfilter_log
dir_4_$(CONFIG_LIB_NETFILTER_CONNTRACK) += libnetfilter_conntrack
dir_4_$(CONFIG_LIB_PWLIB)        += pwlib
dir_4_$(CONFIG_LIB_LIBSDL)       += libsdl # requires libalsa(3)
dir_4_$(CONFIG_LIB_DBUS)         += dbus
dir_4_$(CONFIG_LIB_FUSE)         += fuse
dir_4_$(CONFIG_LIB_UUID)         += uuid
dir_4_$(CONFIG_LIB_TSLIB)        += tslib

dir_5_$(CONFIG_LIB_LIBTINYSDGL)  += TinySDGL     # requires libsdl(4)
dir_5_$(CONFIG_LIB_SDL_GFX)      += SDL_gfx         # requires libsdl(4)
dir_5_$(CONFIG_LIB_SDL_IMAGE)    += SDL_image       # requires libsdl(4) libpng(4) libjpeg(3) zlib (3) tiff(?)
dir_5_$(CONFIG_LIB_SDL_TTF)      += SDL_ttf         # requires libsdl(4) freetype(4)
dir_5_$(CONFIG_LIB_SDL_MIXER)    += SDL_mixer       # requires libsdl(4) tremor(3) SMPEG(?) libmad(3) ...
dir_5_$(CONFIG_LIB_SDL_SOUND)    += SDL_sound       # requires libsdl(4) tremor(3) SMPEG(?) libmad(3) ...
dir_5_$(CONFIG_LIB_FONTCONFIG)   += fontconfig
dir_5_$(CONFIG_LIB_GLIB)         += glib
dir_5_$(CONFIG_LIB_DIRECTFB)     += DirectFB

dir_5_$(CONFIG_LIB_LIBOPENH323)  += libopenh323
dir_5_$(CONFIG_LIB_FFMPEG)       += ffmpeg

dir_5_$(CONFIG_LIB_GCRYPT)       += libgcrypt     # requires libgpg-error(4)


dir_6_$(CONFIG_LIB_LIBOIL)       += liboil
dir_6_$(CONFIG_LIB_LIBSOUP)	 += libsoup
dir_6_$(CONFIG_LIB_LITE)         += LiTE
dir_6_$(CONFIG_LIB_GSTREAMER)    += gstreamer
dir_6_$(CONFIG_LIB_ATK)          += atk
dir_6_$(CONFIG_LIB_CAIRO)        += cairo
dir_6_$(CONFIG_LIB_LIBGEE)       += libgee
dir_6_$(CONFIG_LIB_VLC)          += vlc

dir_6_$(CONFIG_LIB_SSH2)         += libssh2       # requires libgcrypt(5)
dir_6_$(CONFIG_LIB_GNUTLS)       += GnuTLS        # requires libgcrypt(5)
dir_6_$(CONFIG_LIB_DBUS_GLIB)    += dbus-glib     # requires glib, expat, dbus

dir_7_$(CONFIG_LIB_PANGO)        += pango
dir_7_$(CONFIG_LIB_LIBCURL)      += libcurl       # requires libssh2(6) GnuTLS(6)
dir_7_$(CONFIG_LIB_GSSDP)        += gssdp
dir_7_$(CONFIG_LIB_GST-PLUGINS-BASE)    += gst-plugins-base

dir_8_$(CONFIG_LIB_GTK)          += gtk
dir_8_$(CONFIG_LIB_GUPNP)        += gupnp
dir_8_$(CONFIG_LIB_LIBSHOUTCAST) += libshoutcast
dir_8_$(CONFIG_LIB_LIBPANDORA)   += libpandora
dir_8_$(CONFIG_LIB_LIBRECIVA)   += libreciva
dir_8_$(CONFIG_LIBIPOD_DOCK)	+= libipod_dock
dir_8_$(CONFIG_LIB_OGG)         += libogg

dir_9_$(CONFIG_LIB_GUPNP_AV)     += gupnp-av

dir_9_$(CONFIG_LIB_FLAC)	+= FLAC
dir_9_$(CONFIG_LIB_VORBIS)	+= libvorbis

#
# dir 6, 7 and 8 reserved for Makefile.local
#
-include Makefile.local

# And build libraries in the prop directory last
dir_10_y += $(ROOTDIR)/prop

DIRS_NUM = 1 2 3 4 5 6 7 8 9 10
DIRS_y   = $(foreach n,$(DIRS_NUM),$(sort $(dir_$(n)_y)))
DIRS_all = $(foreach n,$(DIRS_NUM),$(sort $(dir_$(n)_y) $(dir_$(n)_n) $(dir_$(n)_)))

##############################################################################

all:
	$(MAKE) -j1 -C ../include                 || exit $$?
	[ -z "$(dir_1_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_1_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_2_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_2_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_3_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_3_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_4_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_4_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_5_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_5_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_6_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_6_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_7_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_7_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_8_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_8_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_9_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_9_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?
	[ -z "$(dir_10_y)" ] ||\
		$(MAKE) -j$(HOST_NCPU) $(sort $(dir_10_y)) || exit $$?
	$(MAKE) -j1 -C ../include
	$(MAKE) make_links                        || exit $$?

# the actual dir target
.PHONY: $(DIRS_y)
$(DIRS_y):
	[ ! -d "$@" ] || ( touch $@/.sgbuilt_lib && $(MAKE) -j1 -C $@ ) || exit $$?

%_only:
	touch $(@:_only=)/.sgbuilt_lib && $(MAKE) -j1 -C $(@:_only=)

%_clean:
	$(MAKE) -j1 -C $(@:_clean=) clean; rm -f $(@:_clean=)/.sgbuilt_lib; true

%_romfs:
	$(MAKE) -j1 -C $(@:_romfs=) romfs

##############################################################################

# Not the best place to install these symlinks, but it will do for now
romfs:
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -s config/passwd /etc/passwd
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -s config/group /etc/group
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -s config/mtab /etc/mtab
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -s config/fstab /etc/fstab
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -s config/hosts /etc/hosts
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -s config/resolv.conf /etc/resolv.conf
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -e CONFIG_USER_BASH_BASH -s config/profile /etc/profile
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD -e CONFIG_USER_TINYLOGIN_SHADOWPASSWDS -s config/shadow /etc/shadow
	for i in $(DIRS_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done

##############################################################################

clean: clean_links
	-for i in $(DIRS_all); do \
		if [ -f $$i/.sgbuilt_lib ]; then \
			$(MAKE) -C $$i clean ; \
			rm -f $$i/.sgbuilt_lib; \
		fi; \
	done

##############################################################################
#
# The libs we link into this directory
#
#
LINKLIBS=\
	libjpeg/*.a \
	libjpeg/.libs/*.so* \
	libm/*.a \
	libatm/src/lib/.libs/*.a \
	libpam/build/libpam*/.libs/*.a \
	libpam/build/libpam*/.libs/*.so \
	libpcap/build/*.a \
	libnet/*.a \
	liblzma/*.a \
	libares/build/*.a \
	libidn/build/lib/.libs/libidn.a \
	libcurl/build/lib/.libs/libcurl.a \
	libcrashlog/libcrashlog.so* \
	libcrashlog/libcrashlog.a \
	libjudy/src/obj/.libs/libJudy.a \
	libjudy/src/obj/.libs/libJudy.so \
	libssl/build/*.so* \
	libssl/build/*.a \
	zlib/libz.a* \
	zlib/libz.so* \
	libbzip2/libbz2.a* \
	libbzip2/libbz2.so* \
	libatm/src/lib/*.so* libatm/src/lib/*.a \
	libcrypt_old/libcrypt_old.a \
	libldap/builddir/libraries/libldap/.libs/*.a \
	libldap/builddir/libraries/liblber/.libs/*.a \
	pcre/builddir/.libs/*.a \
	bitvector/*.so \
	pcre-7.7/builddir/.libs/*.a \
	pcre-7.8/builddir/.libs/*.a \
	$(ROOTDIR)/prop/acld/libacld.a \
	$(ROOTDIR)/prop/libsnapgear++/*.a $(ROOTDIR)/prop/libsnapgear++/*.so* \
	$(ROOTDIR)/prop/libsnapgear/*.a $(ROOTDIR)/prop/libsnapgear/*.so* \
	$(ROOTDIR)/user/tinytcl/*.a $(ROOTDIR)/user/tinytcl/*.a* \
	$(ROOTDIR)/user/tinytcl/*.a $(ROOTDIR)/user/tinytcl/*.so* \
	$(ROOTDIR)/user/tcl/build/*.a \
	$(ROOTDIR)/prop/config/*.a $(ROOTDIR)/prop/config/*.so* \
	$(ROOTDIR)/prop/uberconfig/*.a $(ROOTDIR)/prop/uberconfig/*.so* \
	$(ROOTDIR)/prop/gccmapping/*.a $(ROOTDIR)/prop/gccmapping/*.so* \
	expat/build/.libs/*.a expat/build/.libs/*.so* \
	adns/src/libadns.a \
	flex/build/*.a \
	libgmp/build/install/lib/*.a \
	lzo/build/src/.libs/*.a \
	osip2/build/install/lib/*.a \
	$(ROOTDIR)/prop/libsscep/*.a \
	$(ROOTDIR)/prop/statsd/*.a \
	$(ROOTDIR)/prop/statsd/*.so \
	libefence/*.a \
	termcap/*.a \
	sysfsutils/build/lib/.libs/*.a \
	libnl/lib/*.a* \
	libnl/lib/*.so* \
	libnfnetlink/build/src/.libs/*.a* \
	libnfnetlink/build/src/.libs/*.so* \
	libnfnetlink/build/libnfnetlink.pc \
	libnetfilter_conntrack/build/src/.libs/*.a* \
	libnetfilter_conntrack/build/src/.libs/*.so* \
	libnetfilter_conntrack/build/libnetfilter_conntrack.pc \
	libnetfilter_log/build/src/.libs/*.a* \
	libnetfilter_log/build/src/.libs/*.so* \
	libnetfilter_log/build/libnetfilter_log.pc \
	libiberty/build/*.a \
	bfd/build/*.a \
	popt/build/.libs/*.a \
	STLport/lib/*.a \
	STLport/lib/*.so \
	libffi/install/lib/*.a \
	libffi/install/lib/*.la \
	libffi/install/lib/pkgconfig/*.pc \
	$(ROOTDIR)/prop/libtrustedsource/librep/librep.so* \
	libubidbg/build-ubicom/lib/*.so \
	libshoutcast/build-ubicom/lib/*.so \
	libpandora/build-ubicom/lib/*.so \
	libreciva/build-ubicom/lib/*.so \
	$(ROOTDIR)/prop/librep/librep.a \

# ubicom extensions that are not at all like the rest of uClinux
linklibs_pkgconfig= \
	$(1)/*.so* \
	$(1)/*.a	\
	$(1)/*.la \
	$(1)/pkgconfig/*.pc

linklibs_plain= \
	$(1)/*.so* \
	$(1)/*.a \
	$(1)/*.la

LINKLIBS+= \
	$(call linklibs_pkgconfig,DirectFB/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,atk/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,cairo/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,ffmpeg/build/install/lib) \
	$(call linklibs_pkgconfig,fontconfig/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,freetype/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,glib/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,gstreamer/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,gtk/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libalsa/build-ubicom/lib) \
	$(call linklibs_pkgconfig,libcurl/build/install/lib) \
	$(call linklibs_pkgconfig,libexif/build-ubicom/lib) \
	$(call linklibs_pkgconfig,liboil/build-ubicom/lib) \
	$(call linklibs_pkgconfig,libpng/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libsdl/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libsoup/build-ubicom/lib) \
	$(call linklibs_pkgconfig,libupnp/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libxml2/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,pango/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,pixman/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,sqlite3/build/install/lib) \
	$(call linklibs_pkgconfig,gssdp/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,gupnp/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,gupnp-av/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,dbus-glib/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,gst-plugins-base/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,dbus/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libconfig/build/install/lib) \
	$(call linklibs_pkgconfig,GnuTLS/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libgcrypt/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,TagLib/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,fuse/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libgee/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,tslib/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,vlc/build-ubicom/lib) \
	\
	$(call linklibs_plain,SDL_gfx/build-ubicom/install/lib) \
	$(call linklibs_plain,SDL_image/build-ubicom/install/lib) \
	$(call linklibs_plain,SDL_mixer/build-ubicom/install/lib) \
	$(call linklibs_plain,SDL_sound/build-ubicom/install/lib) \
	$(call linklibs_plain,SDL_ttf/build-ubicom/install/lib) \
	$(call linklibs_plain,gettext/build-ubicom/install/lib) \
	$(call linklibs_plain,libiconv/build-ubicom/install/lib) \
	$(call linklibs_plain,libmad/build-ubicom/lib) \
	$(call linklibs_plain,libid3tag/build-ubicom/lib) \
	$(call linklibs_plain,LiTE/build-ubicom/lib) \
	$(call linklibs_plain,TinySDGL/TinySDGL) \
	$(call linklibs_plain,libssh2/build-ubicom/install/lib) \
	$(call linklibs_plain,libgpg-error/build-ubicom/install/lib) \
	$(call linklibs_plain,popt-1.10.4/build-ubicom/install/lib) \
	$(call linklibs_pkgconfig,libfm/build-ubicom/) \
	$(call linklibs_plain,uuid/build/lib) \
	$(call linklibs_pkgconfig,FLAC/build-ubicom/lib) \
	$(call linklibs_pkgconfig,libogg/build-ubicom/lib) \
	$(call linklibs_pkgconfig,libvorbis/build-ubicom/lib) \
	$(call linklibs_pkgconfig,ffmpeg/build-ubicom/lib)

# The dynamic linker library needs to be linked into lib via a different name
LD_LIB = $(ROOTDIR)/lib/uClibc/lib/ld-uClibc-0.9.30.1.so
LD_LIB_DEST = $(ROOTDIR)/lib/ld-linux.so.2

##############################################################################

make_links: clean_links
	@echo "Making symlinks in lib/"
	@for p in $(LINKLIBS); do \
		for i in $$p; do \
			[ ! -e "$$i" ] && continue; \
			[ "`basename $$i`" = CVS ] && continue; \
			[ -L "./`basename $$i`" ] && continue; \
			ln -s $$i .; \
		done ; \
	done
	@ln -sf $(LD_LIB) $(LD_LIB_DEST)

clean_links:
	@-for i in *; do \
		[ "$$i" = "Makefile" ] && continue; \
		[ "$$i" = "Kconfig" ] && continue; \
		[ -L "$$i" ] || continue ; \
		case "`file -L $$i`" in \
		*directory*) ;; \
		*) rm -f $$i ;; \
		esac ; \
	done

##############################################################################

prune:
	-for i in $(DIRS_all) ; do \
		found=0; \
		for j in $(sort $(DIRS_y)) ; do \
			if [ $$i = $$j ]; then \
				found=1; \
			fi; \
		done; \
		if [ $$found = 0 ]; then \
			rm -fr $$i; \
		fi; \
	done
##############################################################################
