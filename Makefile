include ../../Makefile.param
include ../Makefile.param

include ../../../../Rules.make

PRO_DIR		= $(PRO_HOME_DIR)

TARGET      = hkipc
OBJPATH     = .
SOURCES     = $(wildcard *.c) 
OBJS        = $(patsubst %.c,%.o,$(SOURCES))
FULLOBJS    = $(patsubst %.c,$(OBJPATH)/%.o,$(SOURCES))
TARGETPATH  = /tftpboot/3518e


INCPATH     = -I . -I $(PRO_DIR)/IPCAM_LIB/system/include 		  \
			  -I ../../include 								  \
			  -I ../p2p_server/include 				  \
			  -I ../qq_server/include 				\
			  -I ./recordsdk/ 				  \
			  -I ./wifi_conf/ 				  \
			  -I ./sample_comm 				  \
			  -I ../amr-lib/include/opencore-amrnb/ 		\
			  -I $(PRO_DIR)/Hi3518_SDK_V1.0.8.1/mpp/extdrv/tw2865 \
			  -I $(PRO_DIR)/IPCAM_LIB/openssl_3518/lib_openssl/include \
			  -I $(PRO_DIR)/Hi3518_SDK_V1.0.8.1/drv/hisi-irda	  \
			  -I $(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/include

#LIBPATH     = -L ../lib_so -lsystem -lchinalink -lnetfactory -lutils 
LIBPATH     = -L ../lib_so  -lutils \
			  -L ../p2p_server/lib/arm-hisiv100nptl-linux-gcc -lp2p -lpthread -lm \
			  -L ../amr-lib/lib  \
			  -L ./libs_HI3511 -lrecordSDK -lSampleComm -lwificonfig\
			  -L ../qq_server/lib/  -lpthread -ldl -lssl -lcrypto -lstdc++ \
			  -L ../../lib -lpthread -lm -lmpi -lVoiceEngine -laec -lresampler -lanr -lisp -lsns_ov9712 \
			  -L $(PRO_DIR)/IPCAM_LIB/openssl_3518/lib_openssl/lib -lssl -lcrypto \
			  -L $(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/LibSo -lrtsp -lOnvif
			
MODULES = 	$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetSvRtsp.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetServer.a	\
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/Mod_NetConfig.a	\
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetWebCallBack.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetSvStreamVideo.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetSvStreamAudio.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetClStreamAudio.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_NetDdns.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/LIB_Ddns.a \
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_SysContext.a	\
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_Cgi.a	\
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/MOD_MulLanguage.a	\
			$(PRO_DIR)/IPCAM_LIB/hi3518e_rtsp_web/lib/LIB_WebServer.a	

LINKFLAGS   = -Wall -g
#COMPFLAGS   = -c -O2 -fPIC -lpthread -D_GNU_SOURCE -D_HKIPC -DRTSARM -Wimplicit-function-declaration -Werror
COMPFLAGS   = -c -O2 -fPIC -lpthread -D_GNU_SOURCE -D_HKIPC -DRTSARM -Wimplicit-function-declaration
CXX         = arm-hisiv100nptl-linux-gcc


all:$(TARGET)

$(TARGET):$(OBJS)
	cd ./recordsdk; $(MAKE) -f Makefile.HI3511
	cd ..
	cd ./sample_comm; $(MAKE) -f Makefile.HI3511
	cd ..
	cd ./wifi_conf; $(MAKE) -f Makefile.HI3511
	cd ..
	#$(CXX) $(LINKFLAGS) $(FULLOBJS) -o $(TARGET) $(LIBPATH) $(MODULES)
	$(CXX) $(LINKFLAGS) $(FULLOBJS) -o $(TARGET) $(LIBPATH)
	arm-hisiv100nptl-linux-strip $(TARGET)
	rm -f $(OBJPATH)/.*.swp
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

