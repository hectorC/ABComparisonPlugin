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

#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::Identifier AbcomparisonAudioProcessor::OSCPort = "OSCPort";
const juce::Identifier AbcomparisonAudioProcessor::OSCEnabled = "OSCEnabled";
const juce::Identifier AbcomparisonAudioProcessor::EditorWidth = "editorWidth";
const juce::Identifier AbcomparisonAudioProcessor::EditorHeight = "editorHeight";
const juce::Identifier AbcomparisonAudioProcessor::LabelText = "labelText";
const juce::Identifier AbcomparisonAudioProcessor::ButtonSize = "buttonSize";

//==============================================================================
AbcomparisonAudioProcessor::AbcomparisonAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::discreteChannels (64), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::discreteChannels (64), true)
                     #endif
                       ),
#endif
parameters (*this, nullptr, "ABComparison", createParameters()),
oscReceiver (9222)
{
    for (int choice = 0; choice < maxNChoices; ++choice)
    {
        parameters.addParameterListener ("choiceState" + juce::String (choice), this);
        choiceStates[choice] = parameters.getRawParameterValue ("choiceState" + juce::String (choice));
    }

    parameters.addParameterListener ("numberOfChoices", this);

    parameters.addParameterListener ("fadeTime", this);
    switchMode = parameters.getRawParameterValue ("switchMode");
    fadeTime = parameters.getRawParameterValue ("fadeTime");
    numberOfChoices = parameters.getRawParameterValue ("numberOfChoices");

    oscReceiver.addListener (this, juce::OSCAddress ("/switch"));
}


AbcomparisonAudioProcessor::~AbcomparisonAudioProcessor()
{
}

//==============================================================================
const juce::String AbcomparisonAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AbcomparisonAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AbcomparisonAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AbcomparisonAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AbcomparisonAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AbcomparisonAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AbcomparisonAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AbcomparisonAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AbcomparisonAudioProcessor::getProgramName (int index)
{
    return {};
}

void AbcomparisonAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AbcomparisonAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int choice = 0; choice < maxNChoices; ++choice)
    {
        gains[choice].reset (sampleRate, *fadeTime / 1000.0f);
        gains[choice].setCurrentAndTargetValue (*choiceStates[choice] < 0.5f ? 0.0f : 1.0f);
    }
}

void AbcomparisonAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AbcomparisonAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;
}
#endif

void AbcomparisonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto nCh = buffer.getNumChannels();
    const int stride = *parameters.getRawParameterValue ("channelSize") + 1;
    auto nSamples = buffer.getNumSamples();
    // choice 0
    if (! gains[0].isSmoothing() && gains[0].getTargetValue() == 0.0f)
    {
        for (int ch = 0; ch < juce::jmin (nCh, stride); ++ch)
            buffer.clear (ch, 0, nSamples);
    }
    else
    {
        const float startGain = gains[0].getNextValue();
        gains[0].skip (nSamples - 2);
        const float endGain = gains[0].getNextValue();

        for (int ch = 0; ch < juce::jmin (nCh, stride); ++ch)
            buffer.applyGainRamp (ch, 0, nSamples, startGain, endGain);
    }

    // remaining choices

    const int nChoices = *numberOfChoices + 2;
    for (int choice = 1; choice < nChoices; ++choice)
    {
        if (gains[choice].isSmoothing() || gains[choice].getTargetValue() != 0.0f)
        {
            const float startGain = gains[choice].getNextValue();
            gains[choice].skip (nSamples - 2);
            const float endGain = gains[choice].getNextValue();

            for (int ch = 0; ch < stride; ++ch)
            {
                const int sourceChannel = choice * stride + ch;
                if (sourceChannel < nCh)
                    buffer.addFromWithRamp (ch, 0, buffer.getReadPointer (sourceChannel), nSamples, startGain, endGain);
            }
        }
    }

    // clear not needed channels
    for (int ch = stride; ch < nCh; ++ch)
        buffer.clear (ch, 0, nSamples);

}

//==============================================================================
bool AbcomparisonAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AbcomparisonAudioProcessor::createEditor()
{
    return new AbcomparisonAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void AbcomparisonAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty (OSCPort, oscReceiver.getPortNumber(), nullptr);
    state.setProperty (OSCEnabled, oscReceiver.getAutoConnect(), nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AbcomparisonAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            if (parameters.state.hasProperty (EditorWidth) && parameters.state.hasProperty (EditorHeight))
            {
                editorWidth = parameters.state.getProperty (EditorWidth);
                editorHeight = parameters.state.getProperty (EditorHeight);
                resizeEditorWindow = true;
            }

            if (parameters.state.hasProperty (LabelText))
                setLabelText (parameters.state.getProperty (LabelText));

            if (parameters.state.hasProperty (ButtonSize))
                setButtonSize (parameters.state.getProperty (ButtonSize));

            if (parameters.state.hasProperty (OSCPort))
                oscReceiver.setPort (parameters.state.getProperty (OSCPort));

            if (parameters.state.hasProperty (OSCEnabled))
                oscReceiver.setAutoConnect (parameters.state.getProperty (OSCEnabled));
        }
}

void AbcomparisonAudioProcessor::parameterChanged (const juce::String &parameterID, float newValue)
{
    if (parameterID.startsWith ("choiceState"))
    {
        auto choice = parameterID.substring (11).getIntValue();
        const bool wasOnBefore = gains[choice].getTargetValue() == 1.0f;
        gains[choice].setTargetValue (newValue);
        if (*switchMode < 0.5f && ! mutingOtherChoices) // exclusive solo
        {
            if (wasOnBefore)
                parameters.getParameter ("choiceState" + juce::String (choice))->setValueNotifyingHost (1.0f);
            else
                muteAllOtherChoices (choice);
        }
    }
    else if (parameterID == "fadeTime")
    {
        auto sampleRate = getSampleRate();
        for (int choice = 0; choice < maxNChoices; ++choice)
            gains[choice].reset (sampleRate, *fadeTime / 1000.0f);
    }
    else if (parameterID == "numberOfChoices")
    {
        numberOfChoicesHasChanged = true;
    }
}

void AbcomparisonAudioProcessor::muteAllOtherChoices (const int choiceNotToMute)
{
    juce::ScopedValueSetter<bool> muting (mutingOtherChoices, true);

    for (int i = 0; i < maxNChoices; ++i)
        if (i != choiceNotToMute)
            parameters.getParameter ("choiceState" + juce::String (i))->setValueNotifyingHost (0.0f);
}


void AbcomparisonAudioProcessor::oscMessageReceived (const juce::OSCMessage& msg)
{
    for (auto& arg : msg)
    {
        int choice;
        if (arg.isInt32())
            choice = arg.getInt32();
        else if (arg.isFloat32())
            choice = arg.getFloat32();
        else
            continue;

        choice -= 1; // for `1` changing the first one
        
        if (choice >= 0 && choice < maxNChoices)
        {
            auto param = parameters.getParameter ("choiceState" + juce::String (choice));
            
            if (param->getValue() > 0.5f)
                param->setValueNotifyingHost (0.0f);
            else
                param->setValueNotifyingHost (1.0f);
        }
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout AbcomparisonAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    using Parameter = juce::AudioProcessorValueTreeState::Parameter;
    params.push_back (std::make_unique<Parameter> ("switchMode", "Switch mode", "",
                                                   juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f,
                                                   [](float value)  { return value < 0.5f ? "Exclusive Solo" : "Toggle Mode"; },
                                                   nullptr));

    params.push_back (std::make_unique<Parameter> ("numberOfChoices", "Number of choices", "choices", // has an offset of 2!
                                                    juce::NormalisableRange<float> (0.0f, maxNChoices - 2.0f, 1.0f), 3.0f,
                                                   [](float value) { return juce::String (value + 2, 0); },
                                                   nullptr));

    params.push_back (std::make_unique<Parameter> ("channelSize", "Output Channel Size", "channel (s)", // has an offset of 1!
        juce::NormalisableRange<float> (0.0f, 31.0f, 1.0f), 1.0f, // default is stereo (2 channels)
                                                   [](float value) { return juce::String (value + 1, 0); },
                                                   nullptr));

    params.push_back (std::make_unique<Parameter> ("fadeTime", "Fade-Length", "ms",
        juce::NormalisableRange<float> (0.0f, 1000.0f, 1.0f), 50.0f,
                                                   [](float value) { return juce::String (value); },
                                                   nullptr));

    for (int choice = 0; choice < maxNChoices; ++choice)
        params.push_back (std::make_unique<Parameter> ("choiceState" + juce::String (choice),
                                                       "Choice " + juce::String::charToString (char ('A' + choice)), "",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f,
                                                       [](float value) { return value >= 0.5f ? "ON" :  "OFF"; },
                                                       nullptr, true));

    return { params.begin(), params.end() };
}
//==============================================================================




void AbcomparisonAudioProcessor::setEditorSize (int width, int height)
{
    editorWidth = width;
    editorHeight = height;
    DBG ("Editor called setEditorSize: " << width << "x" << height);

    parameters.state.setProperty (EditorWidth, width, nullptr);
    parameters.state.setProperty (EditorHeight, height, nullptr);
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AbcomparisonAudioProcessor();
}


void AbcomparisonAudioProcessor::setLabelText (juce::String newLabelText)
{
    labelText = newLabelText;
    parameters.state.setProperty (LabelText, labelText, nullptr);

    updateLabelText = true;
}


void AbcomparisonAudioProcessor::setButtonSize (int newSize)
{
    DBG ("setButtonSize");
    buttonSize = newSize;
    parameters.state.setProperty (ButtonSize, newSize, nullptr);

    updateButtonSize = true;
}
