#-------------------------------------------------------------------
#        D - F L A T   M A K E F I L E  -  Borland C++ 3.1 and 4.0
#-------------------------------------------------------------------

all : memopad.exe memopad.hlp

#-------------------------------------------------------------------
#  Delete the TESTING macro to eliminate the Reload
#  Help file selection on the Help menu.
#-------------------------------------------------------------------
TESTING = TESTING_DFLAT
#-------------------------------------------------------------------
#  This macro builds the full D-Flat system with all options enabled.
#  Comment it out for a minimum system or selectively
#  comment out the #defines at the top of dflat.h.
#-------------------------------------------------------------------
FULL = BUILD_FULL_DFLAT
#-------------------------------------------------------------------

#------------------------------------------------
# NOTE: Delete the DEBUG and LINKDEBUG macros to 
# build without debugging information in the .EXE
#------------------------------------------------
DEBUG = -v
LINKDEBUG = /DEB /Map
#------------------------------------------------
COMPILE = cc386 $(DEBUG) -D$(TESTING) -D$(FULL) -DBCPP /C+R /c
LINK= valx -lx -nci $(LINKDEBUG) -Detaillevel:2
#------------------------------------------------

.c.obj:
    $(COMPILE) $*.c

temp.obj: temp.c
	cc386 +c temp.c
memopad.exe : memopad.obj dialogs.obj menus.obj dflat.lib
    $(LINK) c0dosw.obj memopad dialogs menus,memopad,memopad,dflat.lib cldos.lib

dflat.lib :   window.obj video.obj message.obj                         \
              mouse.obj console.obj textbox.obj listbox.obj            \
              normal.obj config.obj menu.obj menubar.obj popdown.obj   \
              rect.obj applicat.obj keys.obj sysmenu.obj editbox.obj   \
              dialbox.obj button.obj fileopen.obj msgbox.obj           \
              helpbox.obj log.obj lists.obj statbar.obj decomp.obj     \
              combobox.obj pictbox.obj calendar.obj barchart.obj       \
              clipbord.obj search.obj dfalloc.obj checkbox.obj         \
              text.obj radio.obj box.obj spinbutt.obj  watch.obj       \
              slidebox.obj direct.obj editor.obj tabbar.obj 
	del dflat.lib
	xlib dflat @dflat.bld

huffc.exe : huffc.obj htree.obj
    $(LINK) -le c0dosw.obj huffc htree,$*.exe,$*,$(LIBS) cldos

fixhelp.exe : fixhelp.obj decomp.obj
    $(LINK) -le c0dosw.obj fixhelp decomp,$*.exe,$*,$(LIBS) cldos

memopad.hlp : memopad.txt huffc.exe fixhelp.exe
    huffc memopad.txt memopad.hlp
    fixhelp memopad