CONFIG += c++14

Release: QMAKE_CFLAGS	+=  -ffast-math -O3
Release: QMAKE_CXXFLAGS	+=  -ffast-math -O3
Release: QMAKE_LFLAGS	+=  -O3

unix:!macx:!android: {
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr
}

win32: {
    CONFIG  += airspy
    CONFIG  += rtl_sdr
    #CONFIG  += soapysdr
}

macx {
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
    ../backend \
    ../audio \
    ../data \
    ../ofdm \
    ../various \
    ../input \
    ../libs/fec

INCLUDEPATH += \
    .. \
    ../backend \
    ../audio \
    ../data \
    ../ofdm \
    ../various \
    ../input \
    ../libs/fec

HEADERS += \
    ../backend/dab-audio.h \
    ../backend/mp2processor.h \
    ../backend/mp4processor.h \
    ../backend/dab_decoder.h \
    ../backend/dabplus_decoder.h \
    ../backend/subchannel_sink.h \
    ../backend/charsets.h \
    ../backend/dab-constants.h \
    ../backend/dab-processor.h \
    ../backend/dab-virtual.h \
    ../backend/mot_manager.h \
    ../backend/pad_decoder.h \
    ../backend/eep-protection.h \
    ../backend/energy_dispersal.h \
    ../backend/fib-processor.h \
    ../backend/fic-handler.h \
    ../backend/msc-handler.h \
    ../backend/freq-interleaver.h \
    ../backend/ofdm-decoder.h \
    ../backend/ofdm-processor.h \
    ../backend/phasereference.h \
    ../backend/phasetable.h \
    ../backend/tii-decoder.h \
    ../backend/protTables.h \
    ../backend/protection.h \
    ../backend/radio-controller.h \
    ../backend/radio-receiver.h \
    ../backend/tools.h \
    ../backend/uep-protection.h \
    ../backend/viterbi.h \\
    ../various/fft.h \
    ../various/ringbuffer.h \
    ../various/Xtan2.h \
    ../various/channels.h \
    ../various/wavfile.h \
    ../various/Socket.h \
    ../various/MathHelper.h \
    ../various/fft.h \
    ../various/ringbuffer.h \
    ../various/Xtan2.h \
    ../various/channels.h \
    ../various/wavfile.h \
    ../various/Socket.h \
    ../input/CVirtualInput.h \
    ../input/CInputFactory.h \
    ../input/CNullDevice.h \
    ../input/CRAWFile.h \
    ../input/CRTL_TCP_Client.h \
    ../various/MathHelper.h \
    ../libs/fec/char.h \
    ../libs/fec/decode_rs.h \
    ../libs/fec/encode_rs.h \
    ../libs/fec/fec.h \
    ../libs/fec/init_rs.h \
    ../libs/fec/rs_common.h

SOURCES += \
    ../backend/dab-audio.cpp \
    ../backend/mp2processor.cpp \
    ../backend/mp4processor.cpp \
    ../backend/dab_decoder.cpp \
    ../backend/dabplus_decoder.cpp \
    ../backend/charsets.cpp \
    ../backend/dab-constants.cpp \
    ../backend/mot_manager.cpp \
    ../backend/pad_decoder.cpp \
    ../backend/eep-protection.cpp \
    ../backend/fib-processor.cpp \
    ../backend/fic-handler.cpp \
    ../backend/msc-handler.cpp \
    ../backend/freq-interleaver.cpp \
    ../backend/ofdm-decoder.cpp \
    ../backend/ofdm-processor.cpp \
    ../backend/phasereference.cpp \
    ../backend/phasetable.cpp \
    ../backend/tii-decoder.cpp \
    ../backend/protTables.cpp \
    ../backend/radio-receiver.cpp \
    ../backend/tools.cpp \
    ../backend/uep-protection.cpp \
    ../backend/viterbi.cpp \
    ../various/Xtan2.cpp \
    ../various/channels.cpp \
    ../various/fft.cpp \
    ../various/wavfile.c \
    ../various/Socket.cpp \
    ../input/CInputFactory.cpp \
    ../input/CNullDevice.cpp \
    ../input/CRAWFile.cpp \
    ../input/CRTL_TCP_Client.cpp \
    ../libs/fec/encode_rs_char.c \
    ../libs/fec/decode_rs_char.c \
    ../libs/fec/init_rs_char.c \


#### Built-in libraries ####
kiss_fft_builtin {
    DEFINES   += KISSFFT

    INCLUDEPATH += ../libs/kiss_fft

    HEADERS    += \
    ../libs/kiss_fft/kiss_fft.h \
    ../libs/kiss_fft/_kiss_fft_guts.h

    SOURCES    += ../libs/kiss_fft/kiss_fft.c
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
    ../libs/faad2 \
    ../libs/faad2/libfaad \
    ../libs/faad2/libfaad/codebook \
    ../libs/faad2/include

    HEADERS += \
    ../libs/faad2/config.h \
    ../libs/faad2/include/faad.h \
    ../libs/faad2/include/neaacdec.h \
    ../libs/faad2/libfaad/analysis.h \
    ../libs/faad2/libfaad/bits.h \
    ../libs/faad2/libfaad/cfft.h \
    ../libs/faad2/libfaad/cfft_tab.h \
    ../libs/faad2/libfaad/common.h \
    ../libs/faad2/libfaad/drc.h \
    ../libs/faad2/libfaad/drm_dec.h \
    ../libs/faad2/libfaad/error.h \
    ../libs/faad2/libfaad/filtbank.h \
    ../libs/faad2/libfaad/fixed.h \
    ../libs/faad2/libfaad/huffman.h \
    ../libs/faad2/libfaad/ic_predict.h \
    ../libs/faad2/libfaad/iq_table.h \
    ../libs/faad2/libfaad/is.h \
    ../libs/faad2/libfaad/kbd_win.h \
    ../libs/faad2/libfaad/lt_predict.h \
    ../libs/faad2/libfaad/mdct.h \
    ../libs/faad2/libfaad/mdct_tab.h \
    ../libs/faad2/libfaad/mp4.h \
    ../libs/faad2/libfaad/ms.h \
    ../libs/faad2/libfaad/output.h \
    ../libs/faad2/libfaad/pns.h \
    ../libs/faad2/libfaad/ps_dec.h \
    ../libs/faad2/libfaad/ps_tables.h \
    ../libs/faad2/libfaad/pulse.h \
    ../libs/faad2/libfaad/rvlc.h \
    ../libs/faad2/libfaad/sbr_dct.h \
    ../libs/faad2/libfaad/sbr_dec.h \
    ../libs/faad2/libfaad/sbr_e_nf.h \
    ../libs/faad2/libfaad/sbr_fbt.h \
    ../libs/faad2/libfaad/sbr_hfadj.h \
    ../libs/faad2/libfaad/sbr_hfgen.h \
    ../libs/faad2/libfaad/sbr_huff.h \
    ../libs/faad2/libfaad/sbr_noise.h \
    ../libs/faad2/libfaad/sbr_qmf_c.h \
    ../libs/faad2/libfaad/sbr_qmf.h \
    ../libs/faad2/libfaad/sbr_syntax.h \
    ../libs/faad2/libfaad/sbr_tf_grid.h \
    ../libs/faad2/libfaad/sine_win.h \
    ../libs/faad2/libfaad/specrec.h \
    ../libs/faad2/libfaad/ssr_fb.h \
    ../libs/faad2/libfaad/ssr.h \
    ../libs/faad2/libfaad/ssr_ipqf.h \
    ../libs/faad2/libfaad/ssr_win.h \
    ../libs/faad2/libfaad/structs.h \
    ../libs/faad2/libfaad/syntax.h \
    ../libs/faad2/libfaad/tns.h \
    ../libs/faad2/libfaad/codebook/hcb_10.h \
    ../libs/faad2/libfaad/codebook/hcb_11.h \
    ../libs/faad2/libfaad/codebook/hcb_1.h \
    ../libs/faad2/libfaad/codebook/hcb_2.h \
    ../libs/faad2/libfaad/codebook/hcb_3.h \
    ../libs/faad2/libfaad/codebook/hcb_4.h \
    ../libs/faad2/libfaad/codebook/hcb_5.h \
    ../libs/faad2/libfaad/codebook/hcb_6.h \
    ../libs/faad2/libfaad/codebook/hcb_7.h \
    ../libs/faad2/libfaad/codebook/hcb_8.h \
    ../libs/faad2/libfaad/codebook/hcb_9.h \
    ../libs/faad2/libfaad/codebook/hcb.h \
    ../libs/faad2/libfaad/codebook/hcb_sf.h

    SOURCES    += \
    ../libs/faad2/libfaad/bits.c \
    ../libs/faad2/libfaad/cfft.c \
    ../libs/faad2/libfaad/common.c \
    ../libs/faad2/libfaad/decoder.c \
    ../libs/faad2/libfaad/drc.c \
    ../libs/faad2/libfaad/drm_dec.c \
    ../libs/faad2/libfaad/error.c \
    ../libs/faad2/libfaad/filtbank.c \
    ../libs/faad2/libfaad/hcr.c \
    ../libs/faad2/libfaad/huffman.c \
    ../libs/faad2/libfaad/ic_predict.c \
    ../libs/faad2/libfaad/is.c \
    ../libs/faad2/libfaad/lt_predict.c \
    ../libs/faad2/libfaad/mdct.c \
    ../libs/faad2/libfaad/mp4.c \
    ../libs/faad2/libfaad/ms.c \
    ../libs/faad2/libfaad/output.c \
    ../libs/faad2/libfaad/pns.c \
    ../libs/faad2/libfaad/ps_dec.c \
    ../libs/faad2/libfaad/ps_syntax.c \
    ../libs/faad2/libfaad/pulse.c \
    ../libs/faad2/libfaad/rvlc.c \
    ../libs/faad2/libfaad/sbr_dct.c \
    ../libs/faad2/libfaad/sbr_dec.c \
    ../libs/faad2/libfaad/sbr_e_nf.c \
    ../libs/faad2/libfaad/sbr_fbt.c \
    ../libs/faad2/libfaad/sbr_hfadj.c \
    ../libs/faad2/libfaad/sbr_hfgen.c \
    ../libs/faad2/libfaad/sbr_huff.c \
    ../libs/faad2/libfaad/sbr_qmf.c \
    ../libs/faad2/libfaad/sbr_syntax.c \
    ../libs/faad2/libfaad/sbr_tf_grid.c \
    ../libs/faad2/libfaad/specrec.c \
    ../libs/faad2/libfaad/ssr.c \
    ../libs/faad2/libfaad/ssr_fb.c \
    ../libs/faad2/libfaad/ssr_ipqf.c \
    ../libs/faad2/libfaad/syntax.c \
    ../libs/faad2/libfaad/tns.c
}

#### Devices ####
airspy {
    DEFINES    += HAVE_AIRSPY
    HEADERS    += ../input/CAirspy.h
    SOURCES    += ../input/CAirspy.cpp

    # The same lib for unix and Windows
    LIBS       += -lairspy
}

rtl_sdr {
    DEFINES    += HAVE_RTLSDR
    HEADERS    += ../input/CRTL_SDR.h
    SOURCES    += ../input/CRTL_SDR.cpp

    # The same lib for unix and Windows
    LIBS       += -lrtlsdr
}

soapysdr {
    DEFINES    += HAVE_SOAPYSDR
    HEADERS    += ../input/CSoapySdr.h
    SOURCES    += ../input/CSoapySdr.cpp

    # The same lib for unix and Windows
    LIBS       += -lSoapySDR
}
