/*
 ==============================================================================

 ABComparison Plug-in
 Copyright (C) 2018 - Daniel Rudrich

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ==============================================================================
 */

/*
 ===== TODOs =====
    - simple MIDI interface?
 */

#pragma once
#include "OSCReceiverPlus.h"
#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/**
*/
class AbcomparisonAudioProcessor :
    public juce::AudioProcessor,
    private juce::AudioProcessorValueTreeState::Listener,
    private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>
{
    static const juce::Identifier EditorWidth;
    static const juce::Identifier EditorHeight;
    static const juce::Identifier OSCPort;
    static const juce::Identifier OSCEnabled;
    static const juce::Identifier LabelText;
    static const juce::Identifier ButtonSize;
    
public:
    //==============================================================================
    static constexpr int maxNChoices = 32;

    //==============================================================================
    AbcomparisonAudioProcessor();
    ~AbcomparisonAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String &parameterID, float newValue) override;
    void muteAllOtherChoices (const int choiceNotToMute);

    //==============================================================================
    void oscMessageReceived (const juce::OSCMessage&) override;


    // === public flag for editor, signaling to resize window
    std::atomic<bool> resizeEditorWindow = false;
    std::atomic<bool> updateLabelText = false;
    std::atomic<bool> updateButtonSize = false;
    void setEditorSize (int width, int height);
    std::atomic<int> editorWidth = 740;
    std::atomic<int> editorHeight = 300;
    std::atomic<bool> numberOfChoicesHasChanged = false;

    void setLabelText (juce::String labelText);
    const juce::String getLabelText() { return labelText; };

    void setButtonSize (int newSize);
    const int getButtonSize() { return buttonSize.get(); };

    OSCReceiverPlus& getOSCReceiver() noexcept { return oscReceiver; }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::LinearSmoothedValue<float> gains[maxNChoices];

    OSCReceiverPlus oscReceiver;

    std::atomic<float>* numberOfChoices;
    std::atomic<float>* switchMode;
    std::atomic<float>* fadeTime;
    std::atomic<float>* choiceStates[maxNChoices];

    bool mutingOtherChoices = false;

    juce::String labelText = "";
    juce::Atomic<int> buttonSize = 120;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbcomparisonAudioProcessor)
};
