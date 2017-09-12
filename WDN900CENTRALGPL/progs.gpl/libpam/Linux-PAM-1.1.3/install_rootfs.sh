#!/bin/sh
rm -vf ${ROOT_FS}/lib/security/pam_unix.so
rm -vf ${ROOT_FS}/lib/libpam.so.0

mkdir -p ${ROOT_FS}/lib/security
${STRIP} -v ./modules/pam_unix/.libs/pam_unix.so -o ${ROOT_FS}/lib/security/pam_unix.so
${STRIP} -v ./libpam/.libs/libpam.so.0 -o ${ROOT_FS}/lib/libpam.so.0
