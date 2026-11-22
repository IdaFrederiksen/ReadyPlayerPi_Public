CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

BUILD    = build

NOTEPI   = ../ClientApp/NotePi
KISSFFT  = ../ThirdParty/kissfft

ALSA_CFLAGS  = $(shell pkg-config --cflags alsa)
ALSA_LDFLAGS = $(shell pkg-config --libs alsa)

INCLUDES = -I. -I$(NOTEPI) -I$(NOTEPI)/control -I$(NOTEPI)/boundary -I$(NOTEPI)/domain -I$(KISSFFT)

DSP_SRC = \
    $(NOTEPI)/boundary/MicrophoneDriver.cpp \
    $(NOTEPI)/control/SoundAnalyzer.cpp \
    $(NOTEPI)/control/SampleProcessor.cpp \
    $(NOTEPI)/control/FirFilter.cpp \
    $(NOTEPI)/control/FftAnalyzer.cpp \
    $(NOTEPI)/control/NoteDetector.cpp \
    $(NOTEPI)/domain/NoteSequence.cpp

KISSFFT_SRC = \
    $(KISSFFT)/kiss_fft.c \
    $(KISSFFT)/kiss_fftr.c

.PHONY: all clean

all: $(BUILD)/module_test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/module_test: NotePiModuleTest.cpp $(DSP_SRC) $(KISSFFT_SRC) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(ALSA_CFLAGS) \
		NotePiModuleTest.cpp $(DSP_SRC) $(KISSFFT_SRC) \
		$(ALSA_LDFLAGS) -lm -o $(BUILD)/module_test

clean:
	rm -rf $(BUILD)
