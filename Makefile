#
# Makefile
#
# Make file for pwlib library
#
# Portable Windows Library
#
# Copyright (c) 1993-1998 Equivalence Pty. Ltd.
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Portable Windows Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Portions are Copyright (C) 1993 Free Software Foundation, Inc.
# All Rights Reserved.
# 
# Contributor(s): ______________________________________.
#
# $Log: Makefile,v $
# Revision 1.16  2000/11/01 04:39:20  robertj
# Made sure opt is first so frech build works
#
# Revision 1.15  2000/11/01 02:42:46  robertj
# Added optnoshared to build all default target.
#
# Revision 1.14  2000/10/30 05:49:25  robertj
# Made make all do bothdepend both
#
# Revision 1.13  2000/06/26 11:17:19  robertj
# Nucleus++ port (incomplete).
#
# Revision 1.12  2000/04/26 02:50:12  robertj
# Fixed build of correct GUI directory.
#
# Revision 1.11  2000/04/26 01:03:46  robertj
# Removed tarfile creation target, this is done differently now.
#
# Revision 1.10  2000/02/04 19:32:16  craigs
# Added targets for unshared libraries etc
#
# Revision 1.9  1999/11/30 00:22:54  robertj
# Updated documentation for doc++
#
# Revision 1.8  1999/06/09 16:09:20  robertj
# Fixed tarball construction not include windows directories
#
# Revision 1.7  1999/06/09 15:41:18  robertj
# Added better UI to make files.
#
# Revision 1.6  1999/04/22 02:37:00  robertj
# Added history file.
#
# Revision 1.5  1999/03/10 04:26:57  robertj
# More documentation changes.
#
# Revision 1.4  1999/03/09 08:07:00  robertj
# Documentation support.
#
# Revision 1.3  1999/01/22 00:30:45  robertj
# Yet more build environment changes.
#
# Revision 1.2  1999/01/16 23:15:11  robertj
# Added ability to NOT have th gui stuff.
#
# Revision 1.1  1999/01/16 04:00:14  robertj
# Initial revision
#
#

ifeq ($(OSTYPE),Nucleus)
TARGETDIR=Nucleus++
else
TARGETDIR=unix
endif

SUBDIRS := src/ptlib/$(TARGETDIR)
ifneq ($(OSTYPE),Nucleus)
SUBDIRS += tools/asnparser

include make/defaultgui.mak
endif

ifdef GUI_TYPE
SUBDIRS += src/pwlib/$(GUI_TYPE)
endif

all : opt optnoshared bothdepend both


opt :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) opt ;)

debug :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) debug ;)

both :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) both ;)

optshared :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) P_SHAREDLIB=1 opt ;)

debugshared :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) P_SHAREDLIB=1 debug ;)

bothshared :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) P_SHAREDLIB=1 both ;)

optnoshared :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) P_SHAREDLIB=0 opt ;)

debugnoshared :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) P_SHAREDLIB=0 debug ;)

bothnoshared :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) P_SHAREDLIB=0 both ;)

clean :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) clean ;)

optclean :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) optclean ;)

debugclean :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) debugclean ;)

optdepend :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) optdepend ;)

debugdepend :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) debugdepend ;)

bothdepend :
	$(foreach dir,$(SUBDIRS),$(MAKE) -C $(dir) bothdepend ;)

ptlib:
	$(MAKE) -C src/ptlib/$(TARGETDIR) both

docs: 
	doc++ --dir html --tables pwlib.dxx


# End of Makefile
