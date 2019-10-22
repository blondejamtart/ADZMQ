#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
ifeq ($(BUILD_IOCS), YES)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))
iocs_DEPEND_DIRS += zmqApp
endif

include $(TOP)/configure/RULES_TOP


