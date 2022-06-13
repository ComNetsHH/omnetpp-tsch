#
# OMNeT++/OMNEST Makefile for tsch
#
# This file was generated with the command:
#  opp_makemake -f --deep -DINET_IMPORT -I. -I../../inet4/src -I../../omnetpp-rpl/src -I../../omnetpp-rpl -L../../inet4/src -L../../omnetpp-rpl/src -lINET$(D) -lomnetpp-rpl$(D) -Xlinklayer/ieee802154e/sixtisch/clx -Xlinklayer/ieee802154e/sixtisch/blacklisting --mode release
#

# Name of target to be created (-o option)
TARGET = tsch$(D)$(EXE_SUFFIX)
TARGET_DIR = .

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(QTENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)
#USERIF_LIBS = $(QTENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = -I. -I../../inet4/src -I../../omnetpp-rpl/src -I../../omnetpp-rpl

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)../../inet4/src $(LDFLAG_LIBPATH)../../omnetpp-rpl/src  -lINET$(D) -lomnetpp-rpl$(D)

# Output directory
PROJECT_OUTPUT_DIR = out
PROJECTRELATIVE_PATH =
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/inet/transportlayer/contract/udp/UdpSocket.o \
    $O/src/applications/pingapp/TschPingApp.o \
    $O/src/applications/tcpapp/TschTcpReSaBasicClientApp.o \
    $O/src/applications/udpapp/ResaUdpVideoStreamClient.o \
    $O/src/applications/udpapp/ResaUdpVideoStreamServer.o \
    $O/src/applications/udpapp/TschUdpBasicApp.o \
    $O/src/applications/udpapp/TschUdpEchoApp.o \
    $O/src/applications/udpapp/TschUdpReSaBasicApp.o \
    $O/src/applications/udpapp/TschUdpReSaEchoApp.o \
    $O/src/applications/udpapp/TschUdpReSaSinkApp.o \
    $O/src/applications/udpapp/WaicUdpSink.o \
    $O/src/applications/udpapp/smokealarm/ResaSmokeUdpSink.o \
    $O/src/common/TschSimsignals.o \
    $O/src/linklayer/ieee802154e/Ieee802154eASN.o \
    $O/src/linklayer/ieee802154e/Ieee802154eMac.o \
    $O/src/linklayer/ieee802154e/TschCSMA.o \
    $O/src/linklayer/ieee802154e/TschHopping.o \
    $O/src/linklayer/ieee802154e/TschLink.o \
    $O/src/linklayer/ieee802154e/TschNeighbor.o \
    $O/src/linklayer/ieee802154e/TschParser.o \
    $O/src/linklayer/ieee802154e/TschSlotframe.o \
    $O/src/linklayer/ieee802154e/TschVirtualLink.o \
    $O/src/linklayer/ieee802154e/sixtisch/Tsch6topSublayer.o \
    $O/src/linklayer/ieee802154e/sixtisch/TschLinkInfo.o \
    $O/src/linklayer/ieee802154e/sixtisch/TschMSF.o \
    $O/src/linklayer/ieee802154e/sixtisch/TschSpectrumSensing.o \
    $O/src/linklayer/ieee802154e/sixtisch/blacklisting/TschBlacklistManager.o \
    $O/src/linklayer/ieee802154e/sixtisch/blacklisting/TschSFSB.o \
    $O/src/linklayer/ieee802154e/sixtisch/clx/TschCLSF.o \
    $O/src/linklayer/ieee802154e/sixtisch/experimental/TschSFX.o \
    $O/src/mobility/FlexibleGridMobility.o \
    $O/src/mobility/ReSaMobility.o \
    $O/src/physicallayer/obstacleloss/ReSAObstacleLoss.o \
    $O/src/radio/WaicDimensionalAnalogModel.o \
    $O/src/radio/WaicDimensionalSnir.o \
    $O/src/common/VirtualLinkTag_m.o \
    $O/src/linklayer/ieee802154e/Ieee802154eMacHeader_m.o \
    $O/src/linklayer/ieee802154e/TschLink_m.o \
    $O/src/linklayer/ieee802154e/TschVirtualLink_m.o \
    $O/src/linklayer/ieee802154e/sixtisch/SixpDataChunk_m.o \
    $O/src/linklayer/ieee802154e/sixtisch/SixpHeaderChunk_m.o \
    $O/src/linklayer/ieee802154e/sixtisch/tsch6pPiggybackTimeoutMsg_m.o \
    $O/src/linklayer/ieee802154e/sixtisch/tsch6topCtrlMsg_m.o \
    $O/src/linklayer/ieee802154e/sixtisch/tschLinkInfoTimeoutMsg_m.o \
    $O/src/linklayer/ieee802154e/sixtisch/blacklisting/tschBlacklistExpiryMsg_m.o

# Message files
MSGFILES = \
    src/common/VirtualLinkTag.msg \
    src/linklayer/ieee802154e/Ieee802154eMacHeader.msg \
    src/linklayer/ieee802154e/TschLink.msg \
    src/linklayer/ieee802154e/TschVirtualLink.msg \
    src/linklayer/ieee802154e/sixtisch/SixpDataChunk.msg \
    src/linklayer/ieee802154e/sixtisch/SixpHeaderChunk.msg \
    src/linklayer/ieee802154e/sixtisch/tsch6pPiggybackTimeoutMsg.msg \
    src/linklayer/ieee802154e/sixtisch/tsch6topCtrlMsg.msg \
    src/linklayer/ieee802154e/sixtisch/tschLinkInfoTimeoutMsg.msg \
    src/linklayer/ieee802154e/sixtisch/blacklisting/tschBlacklistExpiryMsg.msg

# SM files
SMFILES =

# Default mode (-M option); can be overridden with make MODE=debug (or =release)
ifndef MODE
MODE = release
endif

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = $(OPPMAIN_LIB) $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)
ifneq ($(TOOLCHAIN_NAME),clangc2)
LIBS += -Wl,-rpath,$(abspath ../../inet4/src) -Wl,-rpath,$(abspath ../../omnetpp-rpl/src)
endif

COPTS = $(CFLAGS) $(IMPORT_DEFINES) -DINET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET_DIR)/$(TARGET)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clangc2)
	$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib)
endif

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating executable: $@
	$(Q)$(CXX) $(LDFLAGS) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY: all clean cleanall depend msgheaders smheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_DIR)/$(TARGET)
	$(Q)-rm -f $(TARGET_DIR)/$(TARGET:%.dll=%.lib)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(MAKE) -s clean MODE=release
	$(Q)$(MAKE) -s clean MODE=debug
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

# include all dependencies
-include $(OBJS:%.o=%.d)
