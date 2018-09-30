CONFIG += c++14

Release: QMAKE_CFLAGS	+=  -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -O3

DEFINES += DABLIN_AAC_FAAD2

unix:!macx:!android: {
    INCLUDEPATH	+= /usr/local/include
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    LIBS    += -lmp3lame
    LIBS    += -lmpg123
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr
}

win32: {
    INCLUDEPATH += ../../../welle.io-win-libs/include
    LIBS    += -L../../../welle.io-win-libs/x86
    LIBS    += -lfftw3f-3
    LIBS    += -lole32
    LIBS    += -lwinpthread
    LIBS    += -lwinmm
    LIBS    += -lstdc++
    LIBS    += -lws2_32
    LIBS    += -llibfaad
    LIBS    += -lmp3lame
    LIBS    += -lmpg123-0
    LIBS    += -lusb-1.0
    LIBS    += -lws2_32
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr
}

macx {
    INCLUDEPATH	+= /opt/local/include
    INCLUDEPATH	+= /usr/local/include
    LIBS    += -L/opt/local/lib
    LIBS    += -L/usr/local/lib
    LIBS    += -lfftw3f
    LIBS    += -lusb-1.0
    LIBS    += -ldl
    LIBS    += -lfaad
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr
}

android {
    CONFIG  += kiss_fft_builtin
    CONFIG  += libfaad_builtin
}


DEPENDPATH += \
    .. \
    $$PWD/backend \
    $$PWD/audio \
    $$PWD/data \
    $$PWD/ofdm \
    $$PWD/various \
    $$PWD/input \
    $$PWD/libs/fec

INCLUDEPATH += \
    .. \
    $$PWD/backend \
    $$PWD/audio \
    $$PWD/data \
    $$PWD/ofdm \
    $$PWD/various \
    $$PWD/input \
    $$PWD/libs/fec

HEADERS += \
    $$PWD/backend/dab-audio.h \
    $$PWD/backend/dab_decoder.h \
    $$PWD/backend/dabplus_decoder.h \
    $$PWD/backend/subchannel_sink.h \
    $$PWD/backend/charsets.h \
    $$PWD/backend/dab-constants.h \
    $$PWD/backend/dab-processor.h \
    $$PWD/backend/dab-virtual.h \
    $$PWD/backend/mot_manager.h \
    $$PWD/backend/pad_decoder.h \
    $$PWD/backend/eep-protection.h \
    $$PWD/backend/energy_dispersal.h \
    $$PWD/backend/fib-processor.h \
    $$PWD/backend/fic-handler.h \
    $$PWD/backend/msc-handler.h \
    $$PWD/backend/freq-interleaver.h \
    $$PWD/backend/ofdm-decoder.h \
    $$PWD/backend/ofdm-processor.h \
    $$PWD/backend/phasereference.h \
    $$PWD/backend/phasetable.h \
    $$PWD/backend/tii-decoder.h \
    $$PWD/backend/protTables.h \
    $$PWD/backend/protection.h \
    $$PWD/backend/radio-controller.h \
    $$PWD/backend/radio-receiver.h \
    $$PWD/backend/tools.h \
    $$PWD/backend/uep-protection.h \
    $$PWD/backend/viterbi.h \\
    $$PWD/various/fft.h \
    $$PWD/various/ringbuffer.h \
    $$PWD/various/Xtan2.h \
    $$PWD/various/channels.h \
    $$PWD/various/wavfile.h \
    $$PWD/various/Socket.h \
    $$PWD/various/MathHelper.h \
    $$PWD/various/fft.h \
    $$PWD/various/ringbuffer.h \
    $$PWD/various/Xtan2.h \
    $$PWD/various/channels.h \
    $$PWD/various/wavfile.h \
    $$PWD/various/Socket.h \
    $$PWD/various/MathHelper.h \
    $$PWD/libs/fec/char.h \
    $$PWD/libs/fec/decode_rs.h \
    $$PWD/libs/fec/encode_rs.h \
    $$PWD/libs/fec/fec.h \
    $$PWD/libs/fec/init_rs.h \
    $$PWD/libs/fec/rs-common.h \
    $$PWD/backend/decoder_adapter.h \
    $$PWD/input/input_factory.h \
    $$PWD/input/null_device.h \
    $$PWD/input/raw_file.h \
    $$PWD/input/virtual_input.h \
    $$PWD/input/rtl_tcp.h
	
SOURCES += \
    $$PWD/backend/dab-audio.cpp \
    $$PWD/backend/dab_decoder.cpp \
    $$PWD/backend/dabplus_decoder.cpp \
    $$PWD/backend/charsets.cpp \
    $$PWD/backend/dab-constants.cpp \
    $$PWD/backend/mot_manager.cpp \
    $$PWD/backend/pad_decoder.cpp \
    $$PWD/backend/eep-protection.cpp \
    $$PWD/backend/fib-processor.cpp \
    $$PWD/backend/fic-handler.cpp \
    $$PWD/backend/msc-handler.cpp \
    $$PWD/backend/freq-interleaver.cpp \
    $$PWD/backend/ofdm-decoder.cpp \
    $$PWD/backend/ofdm-processor.cpp \
    $$PWD/backend/phasereference.cpp \
    $$PWD/backend/phasetable.cpp \
    $$PWD/backend/tii-decoder.cpp \
    $$PWD/backend/protTables.cpp \
    $$PWD/backend/radio-receiver.cpp \
    $$PWD/backend/tools.cpp \
    $$PWD/backend/uep-protection.cpp \
    $$PWD/backend/viterbi.cpp \
    $$PWD/various/Xtan2.cpp \
    $$PWD/various/channels.cpp \
    $$PWD/various/fft.cpp \
    $$PWD/various/wavfile.c \
    $$PWD/various/Socket.cpp \
    $$PWD/libs/fec/encode_rs_char.c \
    $$PWD/libs/fec/decode_rs_char.c \
    $$PWD/libs/fec/init_rs_char.c \
    $$PWD/backend/decoder_adapter.cpp \
    $$PWD/input/input_factory.cpp \
    $$PWD/input/null_device.cpp \
    $$PWD/input/raw_file.cpp \
    $$PWD/input/rtl_tcp.cpp


#### Built-in libraries ####
kiss_fft_builtin {
    DEFINES   += KISSFFT

    INCLUDEPATH += $$PWD/libs/kiss_fft

    HEADERS    += \
    $$PWD/libs/kiss_fft/kiss_fft.h \
    $$PWD/libs/kiss_fft/_kiss_fft_guts.h

    SOURCES    += $$PWD/libs/kiss_fft/kiss_fft.c
}

libfaad_builtin {
    DEFINES += HAVE_CONFIG_H

    # Dangerous but libfaad produces a lot of warnings
    QMAKE_CFLAGS += -Wno-unused-parameter
    QMAKE_CFLAGS += -Wno-unused-function
    QMAKE_CFLAGS += -Wno-unused-variable
    QMAKE_CFLAGS += -Wno-unused-but-set-variable
    QMAKE_CFLAGS += -Wno-old-style-declaration
    QMAKE_CFLAGS += -Wno-missing-braces

    INCLUDEPATH += \
    $$PWD/libs/faad2 \
    $$PWD/libs/faad2/libfaad \
    $$PWD/libs/faad2/libfaad/codebook \
    $$PWD/libs/faad2/include

    HEADERS += \
    $$PWD/libs/faad2/config.h \
    $$PWD/libs/faad2/include/faad.h \
    $$PWD/libs/faad2/include/neaacdec.h \
    $$PWD/libs/faad2/libfaad/analysis.h \
    $$PWD/libs/faad2/libfaad/bits.h \
    $$PWD/libs/faad2/libfaad/cfft.h \
    $$PWD/libs/faad2/libfaad/cfft_tab.h \
    $$PWD/libs/faad2/libfaad/common.h \
    $$PWD/libs/faad2/libfaad/drc.h \
    $$PWD/libs/faad2/libfaad/drm_dec.h \
    $$PWD/libs/faad2/libfaad/error.h \
    $$PWD/libs/faad2/libfaad/filtbank.h \
    $$PWD/libs/faad2/libfaad/fixed.h \
    $$PWD/libs/faad2/libfaad/huffman.h \
    $$PWD/libs/faad2/libfaad/ic_predict.h \
    $$PWD/libs/faad2/libfaad/iq_table.h \
    $$PWD/libs/faad2/libfaad/is.h \
    $$PWD/libs/faad2/libfaad/kbd_win.h \
    $$PWD/libs/faad2/libfaad/lt_predict.h \
    $$PWD/libs/faad2/libfaad/mdct.h \
    $$PWD/libs/faad2/libfaad/mdct_tab.h \
    $$PWD/libs/faad2/libfaad/mp4.h \
    $$PWD/libs/faad2/libfaad/ms.h \
    $$PWD/libs/faad2/libfaad/output.h \
    $$PWD/libs/faad2/libfaad/pns.h \
    $$PWD/libs/faad2/libfaad/ps_dec.h \
    $$PWD/libs/faad2/libfaad/ps_tables.h \
    $$PWD/libs/faad2/libfaad/pulse.h \
    $$PWD/libs/faad2/libfaad/rvlc.h \
    $$PWD/libs/faad2/libfaad/sbr_dct.h \
    $$PWD/libs/faad2/libfaad/sbr_dec.h \
    $$PWD/libs/faad2/libfaad/sbr_e_nf.h \
    $$PWD/libs/faad2/libfaad/sbr_fbt.h \
    $$PWD/libs/faad2/libfaad/sbr_hfadj.h \
    $$PWD/libs/faad2/libfaad/sbr_hfgen.h \
    $$PWD/libs/faad2/libfaad/sbr_huff.h \
    $$PWD/libs/faad2/libfaad/sbr_noise.h \
    $$PWD/libs/faad2/libfaad/sbr_qmf_c.h \
    $$PWD/libs/faad2/libfaad/sbr_qmf.h \
    $$PWD/libs/faad2/libfaad/sbr_syntax.h \
    $$PWD/libs/faad2/libfaad/sbr_tf_grid.h \
    $$PWD/libs/faad2/libfaad/sine_win.h \
    $$PWD/libs/faad2/libfaad/specrec.h \
    $$PWD/libs/faad2/libfaad/ssr_fb.h \
    $$PWD/libs/faad2/libfaad/ssr.h \
    $$PWD/libs/faad2/libfaad/ssr_ipqf.h \
    $$PWD/libs/faad2/libfaad/ssr_win.h \
    $$PWD/libs/faad2/libfaad/structs.h \
    $$PWD/libs/faad2/libfaad/syntax.h \
    $$PWD/libs/faad2/libfaad/tns.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_10.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_11.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_1.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_2.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_3.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_4.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_5.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_6.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_7.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_8.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_9.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb.h \
    $$PWD/libs/faad2/libfaad/codebook/hcb_sf.h

    SOURCES    += \
    $$PWD/libs/faad2/libfaad/bits.c \
    $$PWD/libs/faad2/libfaad/cfft.c \
    $$PWD/libs/faad2/libfaad/common.c \
    $$PWD/libs/faad2/libfaad/decoder.c \
    $$PWD/libs/faad2/libfaad/drc.c \
    $$PWD/libs/faad2/libfaad/drm_dec.c \
    $$PWD/libs/faad2/libfaad/error.c \
    $$PWD/libs/faad2/libfaad/filtbank.c \
    $$PWD/libs/faad2/libfaad/hcr.c \
    $$PWD/libs/faad2/libfaad/huffman.c \
    $$PWD/libs/faad2/libfaad/ic_predict.c \
    $$PWD/libs/faad2/libfaad/is.c \
    $$PWD/libs/faad2/libfaad/lt_predict.c \
    $$PWD/libs/faad2/libfaad/mdct.c \
    $$PWD/libs/faad2/libfaad/mp4.c \
    $$PWD/libs/faad2/libfaad/ms.c \
    $$PWD/libs/faad2/libfaad/output.c \
    $$PWD/libs/faad2/libfaad/pns.c \
    $$PWD/libs/faad2/libfaad/ps_dec.c \
    $$PWD/libs/faad2/libfaad/ps_syntax.c \
    $$PWD/libs/faad2/libfaad/pulse.c \
    $$PWD/libs/faad2/libfaad/rvlc.c \
    $$PWD/libs/faad2/libfaad/sbr_dct.c \
    $$PWD/libs/faad2/libfaad/sbr_dec.c \
    $$PWD/libs/faad2/libfaad/sbr_e_nf.c \
    $$PWD/libs/faad2/libfaad/sbr_fbt.c \
    $$PWD/libs/faad2/libfaad/sbr_hfadj.c \
    $$PWD/libs/faad2/libfaad/sbr_hfgen.c \
    $$PWD/libs/faad2/libfaad/sbr_huff.c \
    $$PWD/libs/faad2/libfaad/sbr_qmf.c \
    $$PWD/libs/faad2/libfaad/sbr_syntax.c \
    $$PWD/libs/faad2/libfaad/sbr_tf_grid.c \
    $$PWD/libs/faad2/libfaad/specrec.c \
    $$PWD/libs/faad2/libfaad/ssr.c \
    $$PWD/libs/faad2/libfaad/ssr_fb.c \
    $$PWD/libs/faad2/libfaad/ssr_ipqf.c \
    $$PWD/libs/faad2/libfaad/syntax.c \
    $$PWD/libs/faad2/libfaad/tns.c
}

#### Devices ####
airspy {
    DEFINES    += HAVE_AIRSPY
    HEADERS    += $$PWD/input/airspy_sdr.h
    SOURCES    += $$PWD/input/airspy_sdr.cpp

    # The same lib for unix and Windows
    LIBS       += -lairspy
}

rtl_sdr {
    DEFINES    += HAVE_RTLSDR
    HEADERS    += $$PWD/input/rtl_sdr.h
    SOURCES    += $$PWD/input/rtl_sdr.cpp

    # The same lib for unix and Windows
    LIBS       += -lrtlsdr
}

soapysdr {
    DEFINES    += HAVE_SOAPYSDR
    HEADERS    += $$PWD/input/soapy_sdr.h
    SOURCES    += $$PWD/input/soapy_sdr.cpp

    # The same lib for unix and Windows
    LIBS       += -lSoapySDR
}
