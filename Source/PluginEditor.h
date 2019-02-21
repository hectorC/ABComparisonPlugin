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


typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//==============================================================================
/**
*/
class AbcomparisonAudioProcessorEditor  : public AudioProcessorEditor, public KeyListener, private Timer
{
public:
    AbcomparisonAudioProcessorEditor (AbcomparisonAudioProcessor&, AudioProcessorValueTreeState&);
    ~AbcomparisonAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void updateNumberOfButtons();

    bool keyPressed (const KeyPress &key, Component *originatingComponent) override;
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AbcomparisonAudioProcessor& processor;
    AudioProcessorValueTreeState& parameters;

    ComboBox cbSwitchMode;
    ComboBox cbChannelSize;
    ComboBox cbNChoices;
    Slider slFadeTime;

    int nChoices = 2;

    ScopedPointer<ComboBoxAttachment> cbSwitchModeAttachment, cbChannelSizeAttachment, cbNChoicesAttachment;
    ScopedPointer<SliderAttachment> slFadeTimeAttachment;

    OwnedArray<TextButton> tbChoice;
    OwnedArray<ButtonAttachment> tbChoiceAttachments;

    FlexBox flexBox;

    TooltipWindow toolTipWin;

    bool editorIsResizing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbcomparisonAudioProcessorEditor)
};
