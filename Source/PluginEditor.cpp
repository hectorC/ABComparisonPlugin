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

//==============================================================================
AbcomparisonAudioProcessorEditor::AbcomparisonAudioProcessorEditor (AbcomparisonAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&p), processor (p), parameters (vts)
{
    toolTipWin.setMillisecondsBeforeTipAppears (200);
    toolTipWin.setOpaque (false);

    addKeyListener (this);

    // create a set of colours
    juce::Array<juce::Colour> cols;
    cols.add (juce::Colours::limegreen);
    cols.add (juce::Colours::orange);
    cols.add (juce::Colours::orangered);
    cols.add (juce::Colours::slateblue);
    cols.add (juce::Colours::magenta);

    addAndMakeVisible (cbSwitchMode);
    cbSwitchMode.addItem ("Exclusive Solo", 1);
    cbSwitchMode.addItem ("Toggle Mode", 2);
    cbSwitchModeAttachment.reset (new ComboBoxAttachment (parameters, "switchMode", cbSwitchMode));

    addAndMakeVisible (cbNChoices);
    cbNChoices.setJustificationType (juce::Justification::centred);
    for (int i = 2; i <= processor.maxNChoices; ++i)
        cbNChoices.addItem (juce::String (i), i - 1);

    cbNChoicesAttachment.reset (new ComboBoxAttachment (parameters, "numberOfChoices", cbNChoices));

    addAndMakeVisible (cbChannelSize);
    cbChannelSize.setJustificationType (juce::Justification::centred);

    for (int i = 1; i <= 32; ++i)
        cbChannelSize.addItem (juce::String (i) + " ch", i);

    cbChannelSizeAttachment.reset (new ComboBoxAttachment (parameters, "channelSize", cbChannelSize));


    addAndMakeVisible (slFadeTime);
    slFadeTimeAttachment.reset (new SliderAttachment (parameters, "fadeTime", slFadeTime));
    slFadeTime.setTooltip ("Cross-fade time (in ms)");
    slFadeTime.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    slFadeTime.setTextValueSuffix (" ms");

    addAndMakeVisible (tbEditLabels);
    tbEditLabels.setButtonText ("Labels");
    tbEditLabels.onClick = [this] () { editLabels(); };

    addAndMakeVisible (tbEnableOSC);
    tbEnableOSC.setButtonText ("");
    tbEnableOSC.setTooltip ("Enables/disables OSC");
    tbEnableOSC.setToggleState (p.getOSCReceiver().getAutoConnect(), juce::dontSendNotification);
    tbEnableOSC.onClick = [&] ()
    {
        const auto enable = tbEnableOSC.getToggleState();
        p.getOSCReceiver().setAutoConnect (enable);
        if (! enable)
            p.getOSCReceiver().disconnect();
    };

    addAndMakeVisible (teOSCPort);
    teOSCPort.setMultiLine (false);
    teOSCPort.setReturnKeyStartsNewLine (false);
    teOSCPort.setReadOnly (false);
    teOSCPort.setScrollbarsShown (true);
    teOSCPort.setJustification (juce::Justification::centred);
    teOSCPort.setTooltip ("The OSC port for receiving switch command. The command should be '/switch i', with i being the choice you want to play.");
    teOSCPort.setText (juce::String (p.getOSCReceiver().getPortNumber()), juce::dontSendNotification);
    teOSCPort.onReturnKey = [&] ()
    {
        const int currentPort = teOSCPort.getText().getIntValue();
        p.getOSCReceiver().setPort (currentPort);
    };

    flexBox.flexWrap = juce::FlexBox::Wrap::wrap;
    flexBox.alignContent = juce::FlexBox::AlignContent::flexStart;

    for (int choice = 0; choice < processor.maxNChoices; ++choice)
    {
        auto handle = tbChoice.add (new juce::TextButton);
        addAndMakeVisible (handle);

        tbChoiceAttachments.add (new ButtonAttachment (parameters, "choiceState" + juce::String (choice), *handle));
        handle->setClickingTogglesState (true);
        handle->setColour (juce::TextButton::buttonOnColourId, cols[choice % cols.size()]);
    }

    updateNumberOfButtons();


    // set the size of the GUI so the number of choices (nChoices) will fit in there
    {
        juce::ScopedValueSetter<bool> preventEditorFromCallingProcessor (editorIsResizing, true, false);
        setResizeLimits (460, 300, 1440, 700);
        setSize (juce::jmin (processor.editorWidth.load(), 1440), juce::jmin (processor.editorHeight.load(), 700));
    }

    updateLabelText();
    startTimer (50);
    processor.getOSCReceiver().addChangeListener (this);
    changeListenerCallback (nullptr);
}

AbcomparisonAudioProcessorEditor::~AbcomparisonAudioProcessorEditor()
{
    processor.getOSCReceiver().removeChangeListener (this);
}

//==============================================================================
void AbcomparisonAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const juce::String title = "ABComparison";

    juce::Font font = juce::Font (30.0f);
    const float titleWidth = font.getStringWidth (title);

    auto bounds = getLocalBounds();
    bounds.removeFromTop (15);
    auto titleRow = bounds.removeFromTop (40);

    g.setFont (font);
    g.setColour (juce::Colours::white);
    g.drawFittedText (title, titleRow, juce::Justification::centredTop, 1);

    const float reduceAmount = 0.5f * (titleRow.getWidth() + titleWidth);
    titleRow.removeFromLeft (static_cast<int> (ceil (reduceAmount)));

    juce::String versionString = "v";
    versionString.append (JucePlugin_VersionString, 6);

    g.setFont (12.0f);
    g.drawFittedText (versionString, titleRow, juce::Justification::topLeft, 1);

    auto headlineRow = bounds.removeFromTop (14);
    headlineRow.removeFromLeft (15);

    g.setFont (14.0f);
    g.drawText ("Choices", headlineRow.removeFromLeft (65), juce::Justification::centred, 1);
    g.drawText (juce::CharPointer_UTF8 ("\xc3\xa0"), headlineRow.removeFromLeft (10), juce::Justification::centred, 1);
    g.drawText ("Channels", headlineRow.removeFromLeft (75), juce::Justification::centred, 1);
    headlineRow.removeFromLeft (10);
    g.drawText ("Switch mode", headlineRow.removeFromLeft (110), juce::Justification::centred, 1);
    headlineRow.removeFromLeft (7);
    g.drawText ("FadeTime", headlineRow.removeFromLeft (120), juce::Justification::centred, 1);
    headlineRow.removeFromLeft (7 + 75 + 10);
    g.drawText ("OSC", headlineRow.removeFromLeft (26), juce::Justification::left, 1);
    g.drawText ("Port", headlineRow.removeFromLeft (70), juce::Justification::centred, 1);
}

void AbcomparisonAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (70);
    bounds.removeFromLeft (15);
    bounds.removeFromRight (15);

    auto settingsArea = bounds.removeFromTop (26);
    cbNChoices.setBounds (settingsArea.removeFromLeft (65));
    settingsArea.removeFromLeft (10);
    cbChannelSize.setBounds (settingsArea.removeFromLeft (75));
    settingsArea.removeFromLeft (10);
    cbSwitchMode.setBounds (settingsArea.removeFromLeft (120));
    settingsArea.removeFromLeft (7);
    slFadeTime.setBounds (settingsArea.removeFromLeft (110).withHeight (45));
    settingsArea.removeFromLeft (7);
    tbEditLabels.setBounds (settingsArea.removeFromLeft (75));
    settingsArea.removeFromLeft (10);
    tbEnableOSC.setBounds (settingsArea.removeFromLeft (26));
    teOSCPort.setBounds (settingsArea.removeFromLeft (70));

    bounds.removeFromTop (30);

    flexBoxArea = bounds;
    flexBox.performLayout (bounds);

    if (! editorIsResizing) // user is
        processor.setEditorSize (getWidth(), getHeight());
}

bool AbcomparisonAudioProcessorEditor::keyPressed (const juce::KeyPress &key, Component *originatingComponent)
{
    auto choice = key.getKeyCode() - 49;
    if (choice == - 1)
        choice = 9;
    if (choice >= 0 && choice < juce::jmin (nChoices, 10))
        tbChoice.getUnchecked (choice)->triggerClick();
    return true;
}

void AbcomparisonAudioProcessorEditor::updateNumberOfButtons()
{
    flexBox.items.clear();
    nChoices = cbNChoices.getSelectedId() + 1;
    const int size = processor.getButtonSize();

    for (int choice = 0; choice < nChoices; ++choice)
    {
        tbChoice.getUnchecked (choice)->setVisible (true);
        flexBox.items.add (juce::FlexItem (size, size, *tbChoice.getUnchecked (choice)).withMargin ({10, 10, 10, 10}));
    }

    for (int choice = nChoices; choice < processor.maxNChoices; ++choice)
        tbChoice.getUnchecked (choice)->setVisible (false);

    repaint();
}

void AbcomparisonAudioProcessorEditor::timerCallback()
{
    if (cbNChoices.getSelectedId() + 1 != nChoices)
        updateNumberOfButtons();

    if (processor.resizeEditorWindow.exchange (false))
    {
        juce::ScopedValueSetter<bool> preventEditorFromCallingProcessor (editorIsResizing, true, false);
        setSize (processor.editorWidth.load(), processor.editorHeight.load());
    }

    if (processor.numberOfChoicesHasChanged.exchange (false))
    {
        updateNumberOfButtons();
        resized();
    }

    if (processor.updateLabelText.exchange (false))
        updateLabelText();

    if (processor.updateButtonSize.exchange (false))
        updateButtonSize();
}

void AbcomparisonAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster *source)
{
    if (processor.getOSCReceiver().getAutoConnect())
    {
        if (processor.getOSCReceiver().isConnected())
            teOSCPort.setColour (juce::TextEditor::outlineColourId, juce::Colours::green);
        else
            teOSCPort.setColour (juce::TextEditor::outlineColourId, juce::Colours::red);
    }
    else
        teOSCPort.setColour (juce::TextEditor::outlineColourId, getLookAndFeel().findColour (juce::TextEditor::outlineColourId));

    teOSCPort.repaint();
}


void AbcomparisonAudioProcessorEditor::editLabels()
{
    auto settings = std::make_unique<SettingsComponent> (processor);
    settings->setSize (300, 250);

    juce::CallOutBox::launchAsynchronously (std::move (settings), tbEditLabels.getScreenBounds(), nullptr);
}

void AbcomparisonAudioProcessorEditor::updateLabelText()
{
    auto labels = juce::StringArray::fromLines (processor.getLabelText());

    const int nButtons = tbChoice.size();

    int i = 0;
    for (; i < juce::jmin (labels.size(), nButtons); ++i)
        tbChoice[i]->setButtonText (labels[i]);

    for (; i < nButtons; ++i)
        tbChoice[i]->setButtonText (juce::String (i + 1));
}

void AbcomparisonAudioProcessorEditor::updateButtonSize()
{
    const int size = processor.getButtonSize();

    for (auto& item : flexBox.items)
    {
        item.width = size;
        item.minWidth = size;
        item.height = size;
        item.minHeight = size;
    }

    flexBox.performLayout (flexBoxArea);
}
