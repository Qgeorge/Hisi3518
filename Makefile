include ../../Makefile.param
include ../Makefile.param

include ./Rules.make

PRO_DIR		= $(PRO_HOME_DIR)

TARGET      = hkipc
OBJPATH     = .
SOURCES     = $(wildcard *.c) 
OBJS        = $(patsubst %.c,%.o,$(SOURCES))
FULLOBJS    = $(patsubst %.c,$(OBJPATH)/%.o,$(SOURCES))
TARGETPATH  = ~/nfsboot


INCPATH     = -I . -I ./exlib/system/include 		  \
			  -I ../../include 								  \
			  -I ./exlib/p2p_server/include 				  \
			  -I ./exlib/record/include 				  \
			  -I ./exlib/libghttp/include 				  \
			  -I ./netsdk/include/ 				  \
			  -I ./wifi_conf/ 				  \
			  -I ./eamil/ 				  \
			  -I ./sample_comm 				  \
			  -I ./exlib/amr-lib/include/opencore-amrnb/ 		\
			  -I ./exlib/openssl_3518/lib_openssl/include	\
			  -I ./exlib/zlib/include   \
			  -I ./wifi_conf/smartconfig/ \
			  -I ./exlib/system/include/ 
#			  -I $(PRO_DIR)/Hi3518_SDK_V1.0.8.1/drv/hisi-irda	  

#LIBPATH     = -L ../lib_so -lsystem -lchinalink -lnetfactory -lutils 
LIBPATH     = -L ./exlib/lib_so  -lutils \
			  -L ./exlib/p2p_server/lib/arm-hisiv100nptl-linux-gcc -lp2p -lpthread -lm \
			  -L ./exlib/record/lib  -lrecord \
			  -L ./exlib/amr-lib/lib \
			  -L ./libs_HI3511  -lSampleComm -lwificonfig -lnethttp \
			  -L ./exlib/libghttp/lib -lghttp \
			  -L ../../lib -lpthread -lm -lmpi -lVoiceEngine -laec -lresampler -lanr -lisp -lsns_sc1045 \
			  -L ./exlib/openssl_3518/lib_openssl/lib -lssl -lcrypto \
			  -L ./exlib/zlib/lib -lzlog 
#			  -L ./wifi_conf/smartconfig -lsmt
LINKFLAGS   = -Wall -g
#COMPFLAGS   = -c -O2 -fPIC -lpthread -D_GNU_SOURCE -D_HKIPC -DRTSARM -Wimplicit-function-declaration -Werror
COMPFLAGS   = -c -O2 -fPIC -lpthread -D_GNU_SOURCE -D_HKIPC -DRTSARM -Wimplicit-function-declaration
CXX         = arm-hisiv100nptl-linux-gcc


all:$(TARGET)

$(TARGET):$(OBJS)
	cd ./sample_comm; $(MAKE) -f Makefile.HI3511
	cd ..
	cd ./wifi_conf; $(MAKE) -f Makefile.HI3511
	cd ..
#	cd ./eamil; $(MAKE) -f Makefile.HI3511
#	cd ..
	cd ./netsdk; $(MAKE) -f Makefile.HI3511
	cd ..
	cd ./netTools; $(MAKE) -f Makefile.HI3511
	cd ..
	#$(CXX) $(LINKFLAGS) $(FULLOBJS) -o $(TARGET) $(LIBPATH) $(MODULES)
	$(CXX) $(LINKFLAGS) $(FULLOBJS) -o $(TARGET) $(LIBPATH)
	arm-hisiv100nptl-linux-strip $(TARGET)
	rm -f $(OBJPATH)/.*.swp
	rm -f $(OBJPATH)/*.o
	rm -f $(OBJPATH)/libs_HI3511/*.a
	rm -f $(OBJPATH)/*.o
	ls -lh $(TARGET)
	cp $(TARGET) $(TARGETPATH)/

$(OBJS):$(SOURCES)
	#$(CXX) $(COMPFLAGS) $*.c -o $(OBJPATH)/$@ $(INCPATH) $(MODULES)
	$(CXX) $(COMPFLAGS) $*.c -o $(OBJPATH)/$@ $(INCPATH)

clr:
	rm -f $(OBJPATH)/*.o
	ctags -Rn ./

clean:
	rm -f $(OBJPATH)/*.o
	rm -f $(OBJPATH)/.*.swp
	rm -f $(TARGET)
	rm -f $(TARGETPATH)/$(TARGET)
	ctags -Rn ./* ../../include/* $(PRO_DIR)/IPCAM_LIB/system/include $(PRO_DIR)/Hi3518_SDK_V1.0.8.1/mpp/component/isp/sensor/ov_9712+ .

