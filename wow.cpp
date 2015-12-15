

#include "MiniSynth.h"
#include "trace.h"

#define LOG_MIDI 1

/* constructor()
	You can initialize variables here.
	You can also allocate memory here as long is it does not
	require the plugin to be fully instantiated. If so, allocate in init()

*/
CMiniSynth::CMiniSynth()
{
	// Added by RackAFX - DO NOT REMOVE
	//
	// initUI() for GUI controls: this must be called before initializing/using any GUI variables
	initUI();
	// END initUI()

	// built in initialization
	m_PlugInName = "MiniSynth";

	// Default to Stereo Operation:
	// Change this if you want to support more/less channels
	m_uMaxInputChannels = 2;
	m_uMaxOutputChannels = 2;

	// use of MIDI controllers to adjust sliders/knobs
	m_bEnableMIDIControl = true;		// by default this is enabled

	// custom GUI stuff
	m_bLinkGUIRowsAndButtons = false;	// change this if you want to force-link

	// DO NOT CHANGE let RackAFX change it for you; use Edit Project to alter
	m_bUseCustomVSTGUI = true;

	// for a user (not RackAFX) generated GUI - advanced you must compile your own resources
	// DO NOT CHANGE let RackAFX change it for you; use Edit Project to alter
	m_bUserCustomGUI = false;

	// output only - SYNTH - plugin DO NOT CHANGE let RackAFX change it for you; use Edit Project to alter
	m_bOutputOnlyPlugIn = true;

	// un-comment this for VST/AU Buffer-style processing
	//m_bWantVSTBuffers = true;
	//m_bWantBuffers = true;

	m_bWantAllMIDIMessages = true;

	// Finish initializations here
	m_dLastNoteFrequency = -1.0;

	// receive on all channels
	m_uMidiRxChannel = MIDI_CH_ALL;

	// load up voices
	for(int i=0; i<MAX_VOICES; i++)
	{
		// --- create voice
		m_pVoiceArray[i] = new CMiniSynthVoice;

		// --- should never happen
		if(!m_pVoiceArray[i]) return;

		// --- global params (MUST BE DONE before setting up mod matrix!)
		m_pVoiceArray[i]->initGlobalParameters(&m_GlobalSynthParams);
	}

	// --- use the first voice to setup the MmM
	m_pVoiceArray[0]->initializeModMatrix(&m_GlobalModMatrix);

	// --- then set the mod matrix cores on the rest of the voices
	for(int i=0; i<MAX_VOICES; i++)
	{
		// --- all matrices share a common core array of matrix rows
		m_pVoiceArray[i]->setModMatrixCore(m_GlobalModMatrix.getModMatrixCore());
	}


	// From COscillator
	//		  0     1     2     3     4    5       6      7
	// enum { SINE, SAW1, SAW2, SAW3, TRI, SQUARE, NOISE, PNOISE };
	m_uOscTypes[0] = (UINT)0; // Sine
	m_uOscTypes[1] = (UINT)1; // Saw
	m_uOscTypes[2] = (UINT)5; // Square
	m_uOscTypes[3] = (UINT)4; // Tri
	m_uOscTypes[4] = (UINT)6; // Noise
	m_uOscTypes[5] = (UINT)7; // PNoise
	
	// From CFilter
	//       0     1     2     3     4     5     6     7     8
	//enum { LPF1, HPF1, LPF2, HPF2, BPF2, BSF2, LPF4, HPF4, BPF4 };
	m_uFilterTypes[0] = (UINT)2; // LP2
	m_uFilterTypes[1] = (UINT)6; // LP4
	m_uFilterTypes[2] = (UINT)4; // BP2
	m_uFilterTypes[3] = (UINT)8; // BP4
	m_uFilterTypes[4] = (UINT)3; // HP2
	m_uFilterTypes[5] = (UINT)7; // HP4

	// --- Mod Routing Memory
	m_uConnections[0][0] = 0;
	m_uConnections[0][1] = 0;
	m_uConnections[0][2] = 0;
	m_uConnections[1][0] = 0;
	m_uConnections[1][1] = 0;
	m_uConnections[1][2] = 0;
	m_uConnections[2][0] = 0;
	m_uConnections[2][1] = 0;
	m_uConnections[2][2] = 0;
	m_uConnections[3][0] = 0;
	m_uConnections[3][1] = 0;
	m_uConnections[3][2] = 0;

	// --- Mod Routing Destinations
	m_uDestinations[0] = NULL;  // none
	m_uDestinations[1] = DEST_ALL_OSC_FO;
	m_uDestinations[2] = DEST_FILTER1_FC;
	m_uDestinations[3] = DEST_FILTER1_Q;
	m_uDestinations[4] = DEST_FILTER2_FC;
	m_uDestinations[5] = DEST_FILTER2_Q;
	m_uDestinations[6] = DEST_DCA_AMP;
	m_uDestinations[7] = DEST_DCA_PAN;
	//m_uDestinations[2] = (UINT)56; // ALL FIL Fc 

	
	m_dIntensityMatrix[0][0] = &dummy;
	m_dIntensityMatrix[0][1] = &m_GlobalSynthParams.voiceParams.dLFO1OscModIntensity;
	m_dIntensityMatrix[0][2] = &m_GlobalSynthParams.voiceParams.dLFO1Filter1ModIntensity;
	m_dIntensityMatrix[0][3] = &m_GlobalSynthParams.voiceParams.dLFO1Filter1QIntensity;
	m_dIntensityMatrix[0][4] = &m_GlobalSynthParams.voiceParams.dLFO1Filter2ModIntensity;
	m_dIntensityMatrix[0][5] = &m_GlobalSynthParams.voiceParams.dLFO1Filter2QIntensity;
	m_dIntensityMatrix[0][6] = &m_GlobalSynthParams.voiceParams.dLFO1DCAAmpModIntensity;
	m_dIntensityMatrix[0][7] = &m_GlobalSynthParams.voiceParams.dLFO1DCAPanModIntensity;

	m_dIntensityMatrix[1][0] = &dummy;
	m_dIntensityMatrix[1][1] = &m_GlobalSynthParams.voiceParams.dLFO3OscModIntensity;
	m_dIntensityMatrix[1][2] = &m_GlobalSynthParams.voiceParams.dLFO3Filter1ModIntensity;
	m_dIntensityMatrix[1][3] = &m_GlobalSynthParams.voiceParams.dLFO3Filter1QIntensity;
	m_dIntensityMatrix[1][4] = &m_GlobalSynthParams.voiceParams.dLFO3Filter2ModIntensity;
	m_dIntensityMatrix[1][5] = &m_GlobalSynthParams.voiceParams.dLFO3Filter2QIntensity;
	m_dIntensityMatrix[1][6] = &m_GlobalSynthParams.voiceParams.dLFO3DCAAmpModIntensity;
	m_dIntensityMatrix[1][7] = &m_GlobalSynthParams.voiceParams.dLFO3DCAPanModIntensity;

	m_dIntensityMatrix[2][0] = &dummy;
	m_dIntensityMatrix[2][1] = &m_GlobalSynthParams.voiceParams.dEG1OscModIntensity;
	m_dIntensityMatrix[2][2] = &m_GlobalSynthParams.voiceParams.dEG1Filter1ModIntensity;
	m_dIntensityMatrix[2][3] = &m_GlobalSynthParams.voiceParams.dEG1Filter1QIntensity;
	m_dIntensityMatrix[2][4] = &m_GlobalSynthParams.voiceParams.dEG1Filter2ModIntensity;
	m_dIntensityMatrix[2][5] = &m_GlobalSynthParams.voiceParams.dEG1Filter2QIntensity;
	m_dIntensityMatrix[2][6] = &m_GlobalSynthParams.voiceParams.dEG1DCAAmpModIntensity;
	m_dIntensityMatrix[2][7] = &m_GlobalSynthParams.voiceParams.dEG1DCAPanModIntensity;

	m_dIntensityMatrix[3][0] = &dummy;
	m_dIntensityMatrix[3][1] = &m_GlobalSynthParams.voiceParams.dEG2OscModIntensity;
	m_dIntensityMatrix[3][2] = &m_GlobalSynthParams.voiceParams.dEG2Filter1ModIntensity;
	m_dIntensityMatrix[3][3] = &m_GlobalSynthParams.voiceParams.dEG2Filter1QIntensity;
	m_dIntensityMatrix[3][4] = &m_GlobalSynthParams.voiceParams.dEG2Filter2ModIntensity;
	m_dIntensityMatrix[3][5] = &m_GlobalSynthParams.voiceParams.dEG2Filter2QIntensity;
	m_dIntensityMatrix[3][6] = &m_GlobalSynthParams.voiceParams.dEG2DCAAmpModIntensity;
	m_dIntensityMatrix[3][7] = &m_GlobalSynthParams.voiceParams.dEG2DCAPanModIntensity;

	// --- AM FX Multiplier
	m_uAMMul[0] = 1.0;
	m_uAMMul[1] = 10.0;
	m_uAMMul[2] = 100.0;
	m_uAMMul[3] = 1000.0;

	// -- PreAllocate
	dLeft = 0.0;
	dRight = 0.0;
	dLeftAccum = 0.0;
	dRightAccum = 0.0;
}


/* destructor()
	Destroy variables allocated in the contructor()

*/
CMiniSynth::~CMiniSynth(void)
{
	// --- delete on master ONLY
	m_GlobalModMatrix.deleteModMatrix();

	// --- delete voices
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
			delete m_pVoiceArray[i];
	}

}

/*
initialize()
	Called by the client after creation; the parent window handle is now valid
	so you can use the Plug-In -> Host functions here (eg sendUpdateUI())
	See the website www.willpirkle.com for more details
*/
bool __stdcall CMiniSynth::initialize()
{
	// --- initialize presets....
	m_uOsc1Wave = SAW;
	m_uOsc2Wave = SQUARE;
	m_uOsc4Wave = TRI;
	m_uFilter2Type = HP4;
	m_uSwitchDelay = ON;
	sendUpdateGUI();
	return true;
}

/* prepareForPlay()
	Called by the client after Play() is initiated but before audio streams

	You can perform buffer flushes and per-run intializations.
	You can check the following variables and use them if needed:

	m_nNumWAVEChannels;
	m_nSampleRate;
	m_nBitDepth;

	NOTE: the above values are only valid during prepareForPlay() and
		  processAudioFrame() because the user might change to another wave file,
		  or use the sound card, oscillators, or impulse response mechanisms

    NOTE: if you allocte memory in this function, destroy it in ::destroy() above
*/
bool __stdcall CMiniSynth::prepareForPlay()
{
	// Add your code here:
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
		{
			m_pVoiceArray[i]->setSampleRate((double)m_nSampleRate);
			m_pVoiceArray[i]->prepareForPlay();
		}
	}

	// --- FX Prepare for play
	m_AMFX.prepareForPlay((double)m_nSampleRate);
	m_ChorusFX.prepareForPlay((double)m_nSampleRate);
	m_DelayFX.prepareForPlay((double)m_nSampleRate);

	// mass update
	update();
	setModIntensityALL();
	userInterfaceChange(151);
	userInterfaceChange(152);
	userInterfaceChange(153);
	userInterfaceChange(154);
	userInterfaceChange(155);
	userInterfaceChange(156);
	userInterfaceChange(157);
	userInterfaceChange(158);
	userInterfaceChange(159);
	userInterfaceChange(160);
	userInterfaceChange(161);
	userInterfaceChange(162);

	// clear
	m_dLastNoteFrequency = -1.0;

	return true;
}

void CMiniSynth::update()
{
	// --- update global parameters
	//
	// Voice:
	m_GlobalSynthParams.voiceParams.dPortamentoTime_mSec = m_dPortamentoTime_mSec;

	// --- ranges
	m_GlobalSynthParams.voiceParams.dOscFoPitchBendModRange = m_nPitchBendRange;

	m_GlobalSynthParams.voiceParams.dOscFoModRange = m_nRangeOsc;
	m_GlobalSynthParams.voiceParams.dFilter1FcRange = m_dRangeF1FC;
	m_GlobalSynthParams.voiceParams.dFilter2FcRange = m_dRangeF2FC;
	m_GlobalSynthParams.voiceParams.dFilter1QRange = m_dRangeF1Q;
	m_GlobalSynthParams.voiceParams.dFilter2QRange = m_dRangeF2Q;

	// --- intensities
	m_GlobalSynthParams.voiceParams.dFilterKeyTrackIntensity = m_dFilterKeyTrackIntensity;

	// --- MIXING OSC
	m_GlobalSynthParams.voiceParams.dMIXOSCA = 1.0 - m_fMIXOSCX;
	m_GlobalSynthParams.voiceParams.dMIXOSCB = m_fMIXOSCX;
	m_GlobalSynthParams.voiceParams.dMIXOSCC = m_fMIXOSCX;
	m_GlobalSynthParams.voiceParams.dMIXOSCD = 1.0 - m_fMIXOSCX;
	m_GlobalSynthParams.voiceParams.dMIXOSCE = m_fMIXOSCY;
	m_GlobalSynthParams.voiceParams.dMIXOSCF = 1.0 - m_fMIXOSCY;

	// --- MIXING FIL
	m_GlobalSynthParams.voiceParams.dMIXFILA = 1.0 - m_fMIXFilter;
	m_GlobalSynthParams.voiceParams.dMIXFILB = m_fMIXFilter;
	m_GlobalSynthParams.voiceParams.bSeries = m_uFilterConnection;

	// Oscillators:
	// --- waveform
	m_GlobalSynthParams.osc1Params.uWaveform = m_uOscTypes[m_uOsc1Wave];
	m_GlobalSynthParams.osc2Params.uWaveform = m_uOscTypes[m_uOsc2Wave];
	m_GlobalSynthParams.osc4Params.uWaveform = m_uOscTypes[m_uOsc4Wave];

	// --- detuning for MiniSynth
	m_GlobalSynthParams.osc1Params.nCents = m_dDetune_cents;
	m_GlobalSynthParams.osc2Params.nCents = -m_dDetune_cents;
	m_GlobalSynthParams.osc4Params.nCents = 0.5*m_dDetune_cents;

	// --- pulse width
	// m_GlobalSynthParams.osc1Params.dPulseWidthControl = m_dPulseWidth_Pct;
	// m_GlobalSynthParams.osc2Params.dPulseWidthControl = m_dPulseWidth_Pct;

	// --- octave
	m_GlobalSynthParams.osc1Params.nOctave = m_nOctaveOsc1;
	m_GlobalSynthParams.osc2Params.nOctave = m_nOctaveOsc2;
	m_GlobalSynthParams.osc3Params.nOctave = m_nOctaveOsc3;
	m_GlobalSynthParams.osc4Params.nOctave = m_nOctaveOsc4;

	// --- semitones
	m_GlobalSynthParams.osc1Params.nSemitones = m_nSemiOsc1;
	m_GlobalSynthParams.osc2Params.nSemitones = m_nSemiOsc2;
	m_GlobalSynthParams.osc3Params.nSemitones = m_nSemiOsc3;
	m_GlobalSynthParams.osc4Params.nSemitones = m_nSemiOsc4;

	// --- OSC3 is Chebyshev
	m_GlobalSynthParams.osc3Params.dHarmonic0 = m_dOsc3H0;
	m_GlobalSynthParams.osc3Params.dHarmonic1 = m_dOsc3H1;
	m_GlobalSynthParams.osc3Params.dHarmonic2 = m_dOsc3H2;
	m_GlobalSynthParams.osc3Params.dHarmonic3 = m_dOsc3H3;
	m_GlobalSynthParams.osc3Params.dHarmonic4 = m_dOsc3H4;
	m_GlobalSynthParams.osc3Params.dHarmonic5 = m_dOsc3H5;
	m_GlobalSynthParams.osc3Params.dHarmonic6 = m_dOsc3H6;
	m_GlobalSynthParams.osc3Params.dHarmonic7 = m_dOsc3H7;

	// Filters:
	// --- Filter1:
	m_GlobalSynthParams.filter1Params.uFilterType = m_uFilterTypes[m_uFilter1Type];
	m_GlobalSynthParams.filter1Params.dFcControl = m_dFc1Control;
	m_GlobalSynthParams.filter1Params.dQControl = m_dQ1Control;

	// --- Filter2:
	m_GlobalSynthParams.filter2Params.uFilterType = m_uFilterTypes[m_uFilter2Type];
	m_GlobalSynthParams.filter2Params.dFcControl = m_dFc2Control;
	m_GlobalSynthParams.filter2Params.dQControl = m_dQ2Control;

	// LFOS:
	// --- LFO1:
	m_GlobalSynthParams.lfo1Params.uWaveform = m_uLFO1Waveform;
	m_GlobalSynthParams.lfo1Params.dOscFo = m_dLFO1Rate;
	m_GlobalSynthParams.lfo1Params.uLFOMode = m_uLFOMode;

	m_GlobalSynthParams.stepLfoParams.dStepActiveSteps = m_nStepSteps;
	m_GlobalSynthParams.stepLfoParams.dOscFo = m_dStepBPM*ONE_OVER_SIXTY;
	m_GlobalSynthParams.stepLfoParams.dStepValue[0] = m_dStep1;
	m_GlobalSynthParams.stepLfoParams.dStepValue[1] = m_dStep2;
	m_GlobalSynthParams.stepLfoParams.dStepValue[2] = m_dStep3;
	m_GlobalSynthParams.stepLfoParams.dStepValue[3] = m_dStep4;
	m_GlobalSynthParams.stepLfoParams.dStepValue[4] = m_dStep5;
	m_GlobalSynthParams.stepLfoParams.dStepValue[5] = m_dStep6;
	m_GlobalSynthParams.stepLfoParams.dStepValue[6] = m_dStep7;
	m_GlobalSynthParams.stepLfoParams.dStepValue[7] = m_dStep8;
	m_GlobalSynthParams.stepLfoParams.dStepValue[8] = m_dStep9;
	m_GlobalSynthParams.stepLfoParams.dStepValue[9] = m_dStep10;
	m_GlobalSynthParams.stepLfoParams.dStepValue[10] = m_dStep11;
	m_GlobalSynthParams.stepLfoParams.dStepValue[11] = m_dStep12;
	m_GlobalSynthParams.stepLfoParams.dStepValue[12] = m_dStep13;
	m_GlobalSynthParams.stepLfoParams.dStepValue[13] = m_dStep14;
	m_GlobalSynthParams.stepLfoParams.dStepValue[14] = m_dStep15;
	m_GlobalSynthParams.stepLfoParams.dStepValue[15] = m_dStep16;

	// Envelope Generators:
	// --- EG1:
	m_GlobalSynthParams.eg1Params.dAttackTime_mSec = m_dEnv1Attack;
	m_GlobalSynthParams.eg1Params.dDecayTime_mSec = m_dEnv1Decay;
	m_GlobalSynthParams.eg1Params.dSustainLevel = m_dEnv1Sustain;
	m_GlobalSynthParams.eg1Params.dReleaseTime_mSec = m_dEnv1Release;
	m_GlobalSynthParams.eg1Params.bResetToZero = (bool)m_uResetToZero;
	m_GlobalSynthParams.eg1Params.bLegatoMode = (bool)m_uLegatoMode;

	// --- EG2:
	m_GlobalSynthParams.eg2Params.dAttackTime_mSec = m_dEnv2Attack;
	m_GlobalSynthParams.eg2Params.dDecayTime_mSec = m_dEnv2Decay;
	m_GlobalSynthParams.eg2Params.dSustainLevel = m_dEnv2Sustain;
	m_GlobalSynthParams.eg2Params.dReleaseTime_mSec = m_dEnv2Release;
	m_GlobalSynthParams.eg2Params.bResetToZero = (bool)m_uResetToZero;
	m_GlobalSynthParams.eg2Params.bLegatoMode = (bool)m_uLegatoMode;

	// --- EG3 (hardwired into our DCA):
	m_GlobalSynthParams.eg3Params.dAttackTime_mSec = m_dDCAAttackTime_mSec;
	m_GlobalSynthParams.eg3Params.dHoldTime_mSec = m_dDCAHoldTime_mSec;
	m_GlobalSynthParams.eg3Params.dDecayTime_mSec = m_dDCADecayTime_mSec;
	m_GlobalSynthParams.eg3Params.dSustainLevel = m_dDCASustainLevel;
	m_GlobalSynthParams.eg3Params.dReleaseTime_mSec = m_dDCAReleaseTime_mSec;
	m_GlobalSynthParams.eg3Params.bResetToZero = (bool)m_uResetToZero;
	m_GlobalSynthParams.eg3Params.bLegatoMode = (bool)m_uLegatoMode;

	// DCA:
	m_GlobalSynthParams.dcaParams.dAmplitude_dB = m_dVolume_dB;
	m_GlobalSynthParams.dcaParams.dPanControl = m_dPan;
	
	// enable/disable mod matrix stuff:
	if(m_uVelocityToAttackScaling == 1)
		m_GlobalModMatrix.enableModMatrixRow(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING, true); // enable
	else
		m_GlobalModMatrix.enableModMatrixRow(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING, false);

	if(m_uNoteNumberToDecayScaling == 1)
		m_GlobalModMatrix.enableModMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING, true); // enable
	else
		m_GlobalModMatrix.enableModMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING, false);

	//  CHANGE THIS TO SPECIFIC FILTERS!
	if(m_uFilterKeyTrack == 1)
		m_GlobalModMatrix.enableModMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK, true); // enable
	else
		m_GlobalModMatrix.enableModMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK, false);

	// FX:

	// --- update master FX delay
	m_DelayFX.setDelayTime_mSec(m_dDelayTime_mSec);
	m_DelayFX.setFeedback_Pct(m_dFeedback_Pct);
	m_DelayFX.setDelayRatio(m_dDelayRatio);
	m_DelayFX.setWetMix(m_dWetMix);
	m_DelayFX.setMode(m_uDelayMode);
	m_DelayFX.update();

	// --- update Wave FX
	m_WaveFX.setMode(m_uDriveType);
	m_WaveFX.setWaveFactor(m_dDriveGain);
	m_WaveFX.setWetMix(m_dDriveMix);
	m_WaveFX.update();

	// --- update Chorus FX
	m_ChorusFX.setDepth(m_dChorusDepth);
	m_ChorusFX.setFeedback_Pct(m_dChorusFeedback);
	m_ChorusFX.setRate(m_dChorusRate);
	m_ChorusFX.setWetMix(m_dChorusMix);
	m_ChorusFX.update();

	// --- update AM FX
	m_AMFX.setMultiplier(m_uAMMul[m_uAMMultiplier]);
	m_AMFX.setRate(m_dAMRate);
	m_AMFX.setMode(m_uAMMode);
	m_AMFX.setWetMix(m_dAMMix);
	m_AMFX.update();
}

/* processAudioFrame

// ALL VALUES IN AND OUT ON THE RANGE OF -1.0 TO + 1.0

LEFT INPUT = pInputBuffer[0];
RIGHT INPUT = pInputBuffer[1]

LEFT INPUT = pInputBuffer[0]
RIGHT OUTPUT = pOutputBuffer[1]

*/
bool __stdcall CMiniSynth::processAudioFrame(float* pInputBuffer, float* pOutputBuffer, UINT uNumInputChannels, UINT uNumOutputChannels)
{
	dLeftAccum = 0.0;
	dRightAccum = 0.0;

	// --- loop and accumulate voices
	for(int i=0; i<MAX_VOICES; i++)
	{
		// --- render synth
		if(m_pVoiceArray[i])
			m_pVoiceArray[i]->doVoice(dLeft, dRight);

		// --- accumulate and scale
		dLeftAccum += 0.25*dLeft;
		dRightAccum += 0.25*dRight;
	}

	// --- add master FX
	//     note: processing in place to save variables
	if (m_uSwitchRing)
	{
		m_AMFX.processAudio(&dLeftAccum, &dRightAccum,
							&dLeftAccum, &dRightAccum);
	}

	if (m_uSwitchDrive)
	{
		m_WaveFX.processAudio(&dLeftAccum, &dRightAccum,  // input values
							  &dLeftAccum, &dRightAccum); // output values
	}

	if (m_uSwitchChorus)
	{
		m_ChorusFX.processAudio(&dLeftAccum, &dRightAccum,
								&dLeftAccum, &dRightAccum);
	}

	if (m_uSwitchDelay)
	{
		m_DelayFX.processAudio(&dLeftAccum, &dRightAccum,  // input values
							   &dLeftAccum, &dRightAccum); // output values
	}

	pOutputBuffer[0] = dLeftAccum;

	// Mono-In, Stereo-Out (AUX Effect)
	if (uNumOutputChannels == 2)
	{
		pOutputBuffer[1] = dRightAccum;
	}

	return true;
}

void CMiniSynth::enableModRoute(UINT source, UINT & newConnection, UINT & oldConnection)
{
	// If current route is different than new route
	if (newConnection != oldConnection)
	{
		// Disconnect the old route
		m_GlobalModMatrix.enableModMatrixRow(source, oldConnection, false);
	}

	// Save new connection
	oldConnection = newConnection;

	// Connect new route
	m_GlobalModMatrix.enableModMatrixRow(source, newConnection, true);
}

void CMiniSynth::disableEGtoAmp(int EGindex, UINT oldConnection)
{
	// in order to disconnect the correct old ModMatrix
	if (oldConnection == DEST_DCA_AMP)
	{
		if (EGindex == 1)
		{
			m_GlobalModMatrix.enableModMatrixRow(SOURCE_EG1, DEST_DCA_AMP, false);
		}
		else
		{
			m_GlobalModMatrix.enableModMatrixRow(SOURCE_EG2, DEST_DCA_AMP, false);
		}
	}
}

void CMiniSynth::setModIntensity(UINT source, UINT destination, double i)
{
	*m_dIntensityMatrix[source][destination] = i;
}

void CMiniSynth::setModIntensityALL()
{
	// modulator -> control -> intensity
	setModIntensity(0, m_uML1AD, m_dIL1);
	setModIntensity(0, m_uML1BD, m_dIL2);
	setModIntensity(0, m_uML1CD, m_dIL3);

	setModIntensity(1, m_uMS1AD, m_dIS1);
	setModIntensity(1, m_uMS1BD, m_dIS2);
	setModIntensity(1, m_uMS1CD, m_dIS3);

	setModIntensity(2, m_uME1AD, m_dIE1);
	setModIntensity(2, m_uME1BD, m_dIE2);
	setModIntensity(2, m_uME1CD, m_dIE3);

	setModIntensity(3, m_uME2AD, m_dIF1);
	setModIntensity(3, m_uME2BD, m_dIF2);
	setModIntensity(3, m_uME2CD, m_dIF3);
}

/* ADDED BY RACKAFX -- DO NOT EDIT THIS CODE!!! ----------------------------------- //
   	**--0x2983--**

	Variable Name                    Index
-----------------------------------------------
	m_nSemiOsc1                       0
	m_dDetune_cents                   1
	m_dFc1Control                     2
	m_dLFO1Rate                       3
	m_dEnv1Attack                     4
	m_dDelayTime_mSec                 6
	m_dFeedback_Pct                   7
	m_dDelayRatio                     8
	m_dWetMix                         9
	m_nSemiOsc2                       10
	m_dPortamentoTime_mSec            11
	m_dQ1Control                      12
	m_dEnv1Decay                      14
	m_fMIXFilter                      16
	m_uLFOMode                        17
	m_fMIXOSCX                        18
	m_fMIXOSCY                        19
	m_nSemiOsc3                       20
	m_dEnv1Sustain                    24
	m_dFc2Control                     25
	m_uDriveType                      26
	m_dDriveGain                      27
	m_dDriveTone                      28
	m_dDriveMix                       29
	m_nSemiOsc4                       30
	m_uLFO1Waveform                   33
	m_dEnv1Release                    34
	m_dQ2Control                      35
	m_dEnv2Attack                     36
	m_dEnv2Decay                      37
	m_dEnv2Sustain                    38
	m_dEnv2Release                    39
	m_dVolume_dB                      100
	m_uLegatoMode                     101
	m_nPitchBendRange                 102
	m_uResetToZero                    103
	m_uFilterKeyTrack                 104
	m_dFilterKeyTrackIntensity        105
	m_uVelocityToAttackScaling        106
	m_uNoteNumberToDecayScaling       107
	m_dOsc3H0                         108
	m_dOsc3H1                         109
	m_dOsc3H2                         110
	m_dOsc3H3                         111
	m_dOsc3H4                         112
	m_dOsc3H5                         113
	m_nOctaveOsc2                     114
	m_nOctaveOsc3                     115
	m_nOctaveOsc4                     116
	m_nOctaveOsc1                     117
	m_dOsc3H6                         118
	m_dOsc3H7                         119
	m_dDCAAttackTime_mSec             120
	m_dDCAHoldTime_mSec               121
	m_dDCADecayTime_mSec              122
	m_dDCAReleaseTime_mSec            123
	m_dDCASustainLevel                124
	m_uOsc1Wave                       125
	m_uOsc2Wave                       126
	m_uOsc4Wave                       127
	m_dStepBPM                        128
	m_nStepSteps                      129
	m_dStep1                          130
	m_dStep2                          131
	m_dStep3                          132
	m_dStep4                          133
	m_dStep5                          134
	m_dStep6                          135
	m_dStep7                          136
	m_dStep8                          137
	m_dStep9                          138
	m_dStep10                         139
	m_dStep11                         140
	m_dStep12                         141
	m_dStep13                         142
	m_dStep14                         143
	m_dStep15                         144
	m_dStep16                         145
	m_dPan                            146
	m_uSwitchDrive                    147
	m_uSwitchRing                     148
	m_uSwitchChorus                   149
	m_uSwitchDelay                    150
	m_uMS1AD                          151
	m_uMS1BD                          152
	m_uMS1CD                          153
	m_uML1AD                          154
	m_uML1BD                          155
	m_uML1CD                          156
	m_uME1AD                          157
	m_uME1BD                          158
	m_uME1CD                          159
	m_uME2AD                          160
	m_uME2BD                          161
	m_uME2CD                          162
	m_dChorusRate                     163
	m_dChorusDepth                    164
	m_dChorusFeedback                 165
	m_dChorusMix                      166
	m_dAMRate                         167
	m_uAMMultiplier                   168
	m_uAMMode                         169
	m_dAMMix                          170
	m_dIL1                            171
	m_dIL2                            172
	m_dIL3                            173
	m_dIS1                            174
	m_dIS2                            175
	m_dIS3                            176
	m_dIE1                            177
	m_dIE2                            178
	m_dIE3                            179
	m_dIF1                            180
	m_dIF2                            181
	m_dIF3                            182
	m_nRangeOsc                       183
	m_dRangeF1FC                      184
	m_dRangeF2FC                      185
	m_dRangeF1Q                       186
	m_dRangeF2Q                       187
	m_uDelayMode                      41
	m_uFilter1Type                    42
	m_uFilter2Type                    43
	m_uFilterConnection               44

	Assignable Buttons               Index
-----------------------------------------------
	B1                                50
	B2                                51
	B3                                52

-----------------------------------------------
Joystick Drop List Boxes          Index
-----------------------------------------------
	 Drop List A                     60
	 Drop List B                     61
	 Drop List C                     62
	 Drop List D                     63

-----------------------------------------------

	**--0xFFDD--**
// ------------------------------------------------------------------------------- */
// Add your UI Handler code here ------------------------------------------------- //
//
bool __stdcall CMiniSynth::userInterfaceChange(int nControlIndex)
{
	switch (nControlIndex)
	{

	// Step LFO Connections 
	case 151: // StepLFO A 
	{
		if ((m_uDestinations[m_uMS1AD] != m_uConnections[1][1] &&
			m_uDestinations[m_uMS1AD] != m_uConnections[1][2]) ||
			m_uMS1AD == None)
		{
			enableModRoute(SOURCE_STEPLFO, m_uDestinations[m_uMS1AD], m_uConnections[1][0]);
		}
		else
		{
			m_uMS1AD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 152: // StepLFO B
	{
		if ((m_uDestinations[m_uMS1BD] != m_uConnections[1][0] &&
			m_uDestinations[m_uMS1BD] != m_uConnections[1][2]) ||
			m_uMS1BD == None )
		{
			enableModRoute(SOURCE_STEPLFO, m_uDestinations[m_uMS1BD], m_uConnections[1][1]);
		}
		else
		{
			m_uMS1BD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 153: // StepLFO C
	{
		if ((m_uDestinations[m_uMS1CD] != m_uConnections[1][0] &&
			m_uDestinations[m_uMS1CD] != m_uConnections[1][1]) ||
			m_uMS1CD == None)
		{
			enableModRoute(SOURCE_STEPLFO, m_uDestinations[m_uMS1CD], m_uConnections[1][2]);
		}
		else
		{
			m_uMS1CD = None;
			sendUpdateGUI();
		}
		break;
	}
	// LFO1 Connections 
	case 154: // LFO1 A 
	{
		if ((m_uDestinations[m_uML1AD] != m_uConnections[0][1] &&
			m_uDestinations[m_uML1AD] != m_uConnections[0][2]) ||
			m_uML1AD == None)
		{
			enableModRoute(SOURCE_LFO1, m_uDestinations[m_uML1AD], m_uConnections[0][0]);
		}
		else
		{
			m_uML1AD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 155: // LFO1 B
	{
		if ((m_uDestinations[m_uML1BD] != m_uConnections[0][0] &&
			m_uDestinations[m_uML1BD] != m_uConnections[0][2]) ||
			m_uML1BD == None)
		{
			enableModRoute(SOURCE_LFO1, m_uDestinations[m_uML1BD], m_uConnections[0][1]);
		}
		else
		{
			m_uML1BD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 156: // LFO1 C
	{
		if ((m_uDestinations[m_uML1CD] != m_uConnections[0][0] &&
			m_uDestinations[m_uML1CD] != m_uConnections[0][1]) ||
			m_uML1CD == None)
		{
			enableModRoute(SOURCE_LFO1, m_uDestinations[m_uML1CD], m_uConnections[0][2]);
		}
		else
		{
			m_uML1CD = None;
			sendUpdateGUI();
		}
		break;
	}
	// ENV1 Connections 
	case 157: // ENV1 A 
	{
		// delete correct connection 
		disableEGtoAmp(1, m_uConnections[2][0]);

		if ((m_uDestinations[m_uME1AD] != m_uConnections[2][1] &&
			m_uDestinations[m_uME1AD] != m_uConnections[2][2]) ||
			m_uME1AD == None)
		{
			// Turn on correct EG output
			if (m_uME1AD == Amp)
			{
				enableModRoute(SOURCE_EG1, m_uDestinations[m_uME1AD], m_uConnections[2][0]);
			}
			else
			{
				enableModRoute(SOURCE_BIASED_EG1, m_uDestinations[m_uME1AD], m_uConnections[2][0]);
			}
		}
		else
		{
			m_uME1AD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 158: // ENV1 B
	{
		// delete correct connection 
		disableEGtoAmp(1, m_uConnections[2][1]);

		if ((m_uDestinations[m_uME1BD] != m_uConnections[2][0] &&
			m_uDestinations[m_uME1BD] != m_uConnections[2][2]) ||
			m_uME1BD == None)
		{
			// Turn on correct EG output
			if (m_uME1BD == Amp)
			{
				enableModRoute(SOURCE_EG1, m_uDestinations[m_uME1BD], m_uConnections[2][1]);
			}
			else
			{
				enableModRoute(SOURCE_BIASED_EG1, m_uDestinations[m_uME1BD], m_uConnections[2][0]);
			}
		}
		else
		{
			m_uME1BD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 159: // ENV1 C
	{
		// delete correct connection 
		disableEGtoAmp(1, m_uConnections[2][2]);

		if ((m_uDestinations[m_uME1CD] != m_uConnections[2][0] &&
			m_uDestinations[m_uME1CD] != m_uConnections[2][1]) ||
			m_uME1CD == None)
		{
			// Turn on correct EG output
			if (m_uME1CD == Amp)
			{
				enableModRoute(SOURCE_EG1, m_uDestinations[m_uME1CD], m_uConnections[2][2]);
			}
			else
			{
				enableModRoute(SOURCE_BIASED_EG1, m_uDestinations[m_uME1CD], m_uConnections[2][2]);
			}
		}
		else
		{
			m_uME1CD = None;
			sendUpdateGUI();
		}
		break;
	}

	// ENV2 Connections 
	case 160: // ENV2 A 
	{
		// delete correct connection 
		disableEGtoAmp(2, m_uConnections[3][0]);

		if ((m_uDestinations[m_uME2AD] != m_uConnections[3][1] &&
			m_uDestinations[m_uME2AD] != m_uConnections[3][2]) ||
			m_uME2AD == None)
		{
			// Turn on correct EG output
			if (m_uME2AD == Amp)
			{
				enableModRoute(SOURCE_EG2, m_uDestinations[m_uME2AD], m_uConnections[3][0]);
			}
			else
			{
				enableModRoute(SOURCE_BIASED_EG2, m_uDestinations[m_uME2AD], m_uConnections[3][0]);
			}
		}
		else
		{
			m_uME2AD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 161: // ENV2 B
	{
		// delete correct connection 
		disableEGtoAmp(2, m_uConnections[3][1]);

		if ((m_uDestinations[m_uME2BD] != m_uConnections[3][0] &&
			m_uDestinations[m_uME2BD] != m_uConnections[3][2]) ||
			m_uME2BD == None)
		{
			// Turn on correct EG output
			if (m_uME2BD == Amp)
			{
				enableModRoute(SOURCE_EG2, m_uDestinations[m_uME2BD], m_uConnections[3][1]);
			}
			else
			{
				enableModRoute(SOURCE_BIASED_EG2, m_uDestinations[m_uME2BD], m_uConnections[3][1]);
			}
		}
		else
		{
			m_uME2BD = None;
			sendUpdateGUI();
		}
		break;
	}
	case 162: // ENV2 C
	{
		// delete correct connection 
		disableEGtoAmp(2, m_uConnections[4][2]);

		if ((m_uDestinations[m_uME2CD] != m_uConnections[3][0] &&
			m_uDestinations[m_uME2CD] != m_uConnections[3][1]) ||
			m_uME2CD == None)
		{
			// Turn on correct EG output
			if (m_uME2CD == Amp)
			{
				enableModRoute(SOURCE_EG2, m_uDestinations[m_uME2CD], m_uConnections[3][2]);
			}
			else
			{
				enableModRoute(SOURCE_BIASED_EG2, m_uDestinations[m_uME2CD], m_uConnections[3][2]);
			}
		}
		else
		{
			m_uME2CD = None;
			sendUpdateGUI();
		}
		break;
	}
	// ALL MOD INTENSITY ROUTINGS
	case 171: 
	case 172:
	case 173:
	case 174:
	case 175:
	case 176:
	case 177:
	case 178:
	case 179:
	case 180:
	case 181:
	case 182:
	{
		setModIntensityALL();
		break;
	}
	default:
		// update all
		update();
		setModIntensityALL();
		break;
	}
	return true;
}
