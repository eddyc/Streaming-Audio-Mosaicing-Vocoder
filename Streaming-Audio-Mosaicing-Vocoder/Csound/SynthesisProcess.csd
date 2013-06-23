<CsoundSynthesizer>
<CsOptions>
-d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 256
nchnls = 2
0dbfs=1


#define WarpPathBase #200#
#define BandGainBase #300#


;based off FileToPvsBuf example written by joachim heintz 2009

opcode TableToPvsBuf, ikk, iiiii

	i_Table, i_FFTSize, i_Overlap, i_WindowSize, i_WindowShape xin
	k_TimeInKCycles timeinstk
	
	if k_TimeInKCycles == 1 then
	
		i_Length = ftlen(i_Table)  / sr
		k_Cycles = i_Length * kr
		k_Count init 0
		
		loop:
			f_FFTIn pvstanal 1, 1, 1, i_Table, 1, 0, 0
			i_Buffer, k_TimePointer pvsbuffer	f_FFTIn, i_Length + (i_FFTSize / sr)
		loop_lt k_Count, 1, k_Cycles, loop
		
		xout	i_Buffer, k_Count, k_TimePointer
	
	endif
endop

gi_FFTSize init 1024
gk_TimePointer init 0
gk_PhasorIndex init 0

instr 1, Init


	gi_Overlap = gi_FFTSize / 4
	i_WindowSize = gi_FFTSize

	k_On init 0
	k_PVSReaderInstances = p4
	i_SegmentFrameCount = p5
	
	if k_On == 0 then
		
		k_Count = 0
		until k_Count == k_PVSReaderInstances do
		
			event "i", "PVSReader", -1, 3600, i_SegmentFrameCount, k_Count
			k_Count = k_Count + 1
		od
		k_On = 1
	endif
	
	
	gi_TimeInSeconds =  (gi_Overlap / sr) * (i_SegmentFrameCount);

endin

instr 2, PVSReader

	i_HopCount = p4
	i_TimeTableNumber = p5 + $WarpPathBase
	i_GainTableNumber = p5 + $BandGainBase
	
	gi_PVSMagnitudes ftgen 0, 0, gi_FFTSize / 2, -2, 0
	
	gk_TimePointer table gk_PhasorIndex , i_TimeTableNumber, 1

	f_ChannelOne pvsfread gk_TimePointer, "PVSInput.pvoc", 0
	f_ChannelTwo pvsfread gk_TimePointer, "PVSInput.pvoc", 0

	k_flag pvsftw f_ChannelOne, gi_PVSMagnitudes
	vmultv gi_PVSMagnitudes, i_GainTableNumber, gi_FFTSize / 2 
	pvsftr f_ChannelOne, gi_PVSMagnitudes

	k_flag pvsftw f_ChannelTwo, gi_PVSMagnitudes
	vmultv gi_PVSMagnitudes, i_GainTableNumber, gi_FFTSize / 2 
	
	pvsftr f_ChannelTwo, gi_PVSMagnitudes

	
;	k_Blurtime = (gi_Overlap / sr)  * 8
;	i_Blurtime = (gi_Overlap / sr)  * 8

;	f_ChannelOneBlurred pvsblur f_ChannelOne, k_Blurtime, i_Blurtime
;	f_ChannelTwoBlurred pvsblur f_ChannelTwo, k_Blurtime, i_Blurtime
;	
;	f_OutL = f_ChannelOneBlurred
;	f_OutR = f_ChannelTwoBlurred

;	f_OutL = f_ChannelOne
;	f_OutR = f_ChannelTwo

	a_ChannelOne pvsynth f_ChannelOne
	a_ChannelTwo pvsynth f_ChannelTwo

	outs a_ChannelOne, a_ChannelTwo
	
	turnon 3

endin

instr 3, Phasor

	gk_PhasorIndex phasor 1 / gi_TimeInSeconds, ((ftlen($WarpPathBase) - 1)/ftlen($WarpPathBase))

endin




</CsInstruments>  
<CsScore>
f 1 0 16384 10 1

e3600
</CsScore>
</CsoundSynthesizer>
