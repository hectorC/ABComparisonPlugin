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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "SettingsComponent.h"

typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//==============================================================================
/**
*/
class AbcomparisonAudioProcessorEditor  : public juce::AudioProcessorEditor,
    public juce::KeyListener,
    private juce::Timer,
    public juce::ChangeListener
{
public:
    AbcomparisonAudioProcessorEditor (AbcomparisonAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AbcomparisonAudioProcessorEditor();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void updateNumberOfButtons();

    bool keyPressed (const juce::KeyPress &key, Component *originatingComponent) override;
    void timerCallback() override;

    void editLabels();
    void updateLabelText();
    void updateButtonSize();

    void changeListenerCallback (juce::ChangeBroadcaster *source) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AbcomparisonAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& parameters;

    juce::ComboBox cbSwitchMode;
    juce::ComboBox cbChannelSize;
    juce::ComboBox cbNChoices;
    juce::Slider slFadeTime;
    juce::ToggleButton tbEnableOSC;
    juce::TextEditor teOSCPort;

    int nChoices = 2;

    std::unique_ptr<ComboBoxAttachment> cbSwitchModeAttachment, cbChannelSizeAttachment, cbNChoicesAttachment;
    std::unique_ptr<SliderAttachment> slFadeTimeAttachment;

    juce::OwnedArray<juce::TextButton> tbChoice;
    juce::OwnedArray<ButtonAttachment> tbChoiceAttachments;

    juce::TextButton tbEditLabels;

    juce::FlexBox flexBox;
    juce::Rectangle<int> flexBoxArea;

    juce::TooltipWindow toolTipWin;

    bool editorIsResizing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbcomparisonAudioProcessorEditor)
};
