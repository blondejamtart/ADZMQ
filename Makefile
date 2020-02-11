#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
ifeq ($(BUILD_IOCS), YES)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))
iocs_DEPEND_DIRS += zmqApp
endif

# Comment out the following lines to disable creation of example iocs and documentation
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard etc))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard docs))

ifeq ($(wildcard etc),etc)
	include $(TOP)/etc/makeIocs/Makefile.iocs
	UNINSTALL_DIRS += documentation/doxygen $(IOC_DIRS)
endif

# Comment out the following line to disable building of example iocs
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))

include $(TOP)/configure/RULES_TOP


