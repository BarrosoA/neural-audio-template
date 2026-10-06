#include "PluginEditor.h"

FlatHeaderLookAndFeel::FlatHeaderLookAndFeel() {
    setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(0xff16181d));
    setColour(juce::PopupMenu::textColourId,                  juce::Colour(0xffd6d9e0));
    setColour(juce::PopupMenu::headerTextColourId,            juce::Colour(0xff6e737d));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff252932));
    setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(0xffffffff));
}

void FlatHeaderLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour& /*backgroundColour*/,
                                                 bool /*shouldDrawButtonAsHighlighted*/,
                                                 bool /*shouldDrawButtonAsDown*/) {
    // flat borderless header buttons: active indicator underline on toggle
    if (button.getToggleState()) {
        auto bounds = button.getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xffeef0f5));
        g.fillRoundedRectangle(bounds.getCentreX() - bounds.getWidth() * 0.18f,
                               bounds.getBottom() - 3.0f,
                               bounds.getWidth() * 0.36f, 1.5f, 0.75f);
    }
}

void FlatHeaderLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat();
    if (shouldDrawButtonAsDown && button.isEnabled())
        bounds.translate(0.0f, 0.5f);

    const juce::String id = button.getComponentID();
    const bool enabled = button.isEnabled();
    const bool toggled = button.getToggleState();

    const juce::Colour baseCol = !enabled ? juce::Colour(0xff434750)
                               : (toggled ? juce::Colour(0xfff2f4f8)
                               : (shouldDrawButtonAsHighlighted ? juce::Colour(0xffffffff) : juce::Colour(0xffb4b8c2)));

    g.setColour(baseCol);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float h  = bounds.getHeight();

    // vector chevrons
    if (id == "prev" || id == "next") {
        const float ch = h * 0.20f;
        const float cw = ch * 0.60f;
        juce::Path p;
        if (id == "prev") {
            p.startNewSubPath(cx + cw * 0.5f, cy - ch);
            p.lineTo(cx - cw * 0.5f, cy);
            p.lineTo(cx + cw * 0.5f, cy + ch);
        } else {
            p.startNewSubPath(cx - cw * 0.5f, cy - ch);
            p.lineTo(cx + cw * 0.5f, cy);
            p.lineTo(cx - cw * 0.5f, cy + ch);
        }
        g.strokePath(p, juce::PathStrokeType(std::max(1.2f, h * 0.065f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    // vector curved arrow for undo/redo
    if (id == "undo" || id == "redo") {
        const float dir = (id == "undo") ? -1.0f : 1.0f;
        const float r = h * 0.19f;
        const float stroke = std::max(1.2f, h * 0.062f);

        juce::Path arc;
        const float tipX = cx + dir * r * 0.95f;
        const float tipY = cy - r * 0.35f;

        arc.startNewSubPath(tipX, tipY);
        arc.lineTo(cx - dir * r * 0.35f, tipY);
        arc.cubicTo(cx - dir * r * 1.15f, tipY, cx - dir * r * 1.15f, cy + r * 0.85f, cx - dir * r * 0.25f, cy + r * 0.85f);
        arc.lineTo(cx + dir * r * 0.75f, cy + r * 0.85f);
        g.strokePath(arc, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float ah = r * 0.55f;
        juce::Path head;
        head.startNewSubPath(tipX - dir * ah, tipY - ah * 0.75f);
        head.lineTo(tipX, tipY);
        head.lineTo(tipX - dir * ah, tipY + ah * 0.75f);
        g.strokePath(head, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    // preset dropdown button
    if (id == "preset") {
        const float fontH = h * 0.46f;
        g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), fontH, juce::Font::plain)));
        g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(0xffffffff) : juce::Colour(0xffdcdfe6));
        auto textBounds = bounds.reduced(h * 0.45f, 0.0f);
        g.drawText(button.getButtonText(), textBounds, juce::Justification::centred, false);

        const float triX = bounds.getRight() - h * 0.40f;
        const float triW = h * 0.12f;
        const float triH = h * 0.07f;
        juce::Path caret;
        caret.addTriangle(triX - triW, cy - triH, triX + triW, cy - triH, triX, cy + triH * 1.2f);
        g.setColour(juce::Colour(0xff6c707a));
        g.fillPath(caret);
        return;
    }

    const float fontH = (id == "copy" || id == "zoom") ? (h * 0.44f) : (h * 0.47f);
    const int styleFlags = toggled ? juce::Font::bold : juce::Font::plain;
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), fontH, styleFlags)));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred, false);
}

void FlatHeaderLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    g.setColour(juce::Colour(0xff15171c));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff2a2e37));
    g.drawRect(bounds, 1.0f);
}

void FlatHeaderLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                              bool isSeparator, bool isActive, bool isHighlighted,
                                              bool isTicked, bool, const juce::String& text,
                                              const juce::String&, const juce::Drawable*,
                                              const juce::Colour*) {
    if (isSeparator) {
        auto r = area.reduced(8, 0);
        g.setColour(juce::Colour(0xff252932));
        g.fillRect(r.getX(), r.getCentreY(), r.getWidth(), 1);
        return;
    }

    auto r = area.reduced(3, 1);
    if (isHighlighted && isActive) {
        g.setColour(juce::Colour(0xff232730));
        g.fillRoundedRectangle(r.toFloat(), 2.5f);
    }

    g.setColour(!isActive ? juce::Colour(0xff555963)
              : (isHighlighted ? juce::Colour(0xffffffff) : (isTicked ? juce::Colour(0xfff0f2f7) : juce::Colour(0xffc5c9d2))));

    g.setFont(getPopupMenuFont());
    auto textRect = r.reduced(10, 0);
    g.drawFittedText(text, textRect, juce::Justification::centredLeft, 1);

    if (isTicked) {
        const float dotR = 2.5f;
        g.setColour(juce::Colour(0xffd84a38));
        g.fillEllipse((float)r.getRight() - 14.0f, (float)r.getCentreY() - dotR, dotR * 2.0f, dotR * 2.0f);
    }
}

juce::Font FlatHeaderLookAndFeel::getPopupMenuFont() {
    return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain));
}

// ==============================================================================
// GenericNeuralAudioProcessorEditor
// ==============================================================================
GenericNeuralAudioProcessorEditor::GenericNeuralAudioProcessorEditor(GenericNeuralAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {

    auto setupHeaderBtn = [&](juce::TextButton& btn, const juce::String& id, const juce::String& tip) {
        btn.setComponentID(id);
        btn.setLookAndFeel(&headerLookAndFeel);
        btn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        btn.setTooltip(tip);
        addAndMakeVisible(btn);
    };

    setupHeaderBtn(presetPrevBtn, "prev",   "Previous Preset");
    setupHeaderBtn(presetMenuBtn, "preset", "Select / Manage Presets");
    setupHeaderBtn(presetNextBtn, "next",   "Next Preset");
    setupHeaderBtn(undoBtn,       "undo",   "Undo");
    setupHeaderBtn(redoBtn,       "redo",   "Redo");
    setupHeaderBtn(stateABtn,     "stateA", "Slot A");
    setupHeaderBtn(stateBBtn,     "stateB", "Slot B");
    setupHeaderBtn(copyStateBtn,  "copy",   "Copy Active Slot to Inactive Slot");
    setupHeaderBtn(zoomBtn,       "zoom",   "UI Zoom Scale");

    presetPrevBtn.onClick = [this]() {
        const auto& presets = GenericNeuralAudioProcessor::getFactoryPresets();
        int count = (int)presets.size();
        int next = (processorRef.getCurrentProgram() - 1 + count) % count;
        processorRef.loadPresetByIndex(next);
        updateHeaderDisplay();
    };

    presetNextBtn.onClick = [this]() {
        const auto& presets = GenericNeuralAudioProcessor::getFactoryPresets();
        int count = (int)presets.size();
        int next = (processorRef.getCurrentProgram() + 1) % count;
        processorRef.loadPresetByIndex(next);
        updateHeaderDisplay();
    };

    presetMenuBtn.onClick = [this]() { showPresetMenu(); };
    undoBtn.onClick       = [this]() { if (processorRef.undoManager.canUndo()) { processorRef.undoManager.undo(); updateHeaderDisplay(); } };
    redoBtn.onClick       = [this]() { if (processorRef.undoManager.canRedo()) { processorRef.undoManager.redo(); updateHeaderDisplay(); } };
    stateABtn.onClick     = [this]() { processorRef.setStateSlot(false); updateHeaderDisplay(); };
    stateBBtn.onClick     = [this]() { processorRef.setStateSlot(true);  updateHeaderDisplay(); };
    copyStateBtn.onClick  = [this]() { processorRef.copyActiveStateToInactive(); copyFlashFrames = 28; updateHeaderDisplay(); };
    zoomBtn.onClick       = [this]() { showZoomMenu(); };

    auto setupKnob = [&](juce::Slider& s, juce::Label& lbl, const juce::String& name) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        s.setTooltip(name);
        addAndMakeVisible(s);

        lbl.setJustificationType(juce::Justification::centred);
        lbl.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold)));
        lbl.setColour(juce::Label::textColourId, juce::Colour(0xff8c9099));
        addAndMakeVisible(lbl);
    };

    setupKnob(driveSlider,  driveLabel,  "Drive");
    setupKnob(outputSlider, outputLabel, "Output Level");

    driveAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "drive", driveSlider);
    outputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "output", outputSlider);

    auto bindPill = [this](juce::Slider& s) {
        s.onDragStart = [this, &s]() {
            processorRef.undoManager.beginNewTransaction("Adjust Parameter");
            updateValuePillForSlider(&s);
            pillHoldFrames = 18;
            pillAlpha = 1.0f;
            repaint();
        };
        s.onValueChange = [this, &s]() {
            if (s.isMouseOverOrDragging() || activePillSlider == &s) {
                updateValuePillForSlider(&s);
                pillHoldFrames = 18;
                pillAlpha = 1.0f;
                repaint();
            }
        };
        s.onDragEnd = [this]() {
            pillHoldFrames = 6;
            updateHeaderDisplay();
        };
    };

    bindPill(driveSlider);
    bindPill(outputSlider);

    updateHeaderDisplay();

    setSize(520, 360);
    setResizable(true, true);
    setResizeLimits(390, 270, 1040, 720);
    startTimerHz(60);
}

GenericNeuralAudioProcessorEditor::~GenericNeuralAudioProcessorEditor() {
    stopTimer();
    presetPrevBtn.setLookAndFeel(nullptr);
    presetMenuBtn.setLookAndFeel(nullptr);
    presetNextBtn.setLookAndFeel(nullptr);
    undoBtn.setLookAndFeel(nullptr);
    redoBtn.setLookAndFeel(nullptr);
    stateABtn.setLookAndFeel(nullptr);
    stateBBtn.setLookAndFeel(nullptr);
    copyStateBtn.setLookAndFeel(nullptr);
    zoomBtn.setLookAndFeel(nullptr);
}

void GenericNeuralAudioProcessorEditor::showPresetMenu() {
    juce::PopupMenu menu;
    menu.setLookAndFeel(&headerLookAndFeel);

    const auto& presets = GenericNeuralAudioProcessor::getFactoryPresets();
    const juce::String curName = processorRef.getCurrentPresetName();

    for (int i = 0; i < (int)presets.size(); ++i) {
        menu.addItem(i + 1, presets[(size_t)i].name, true, curName == presets[(size_t)i].name);
    }

    menu.addSeparator();
    menu.addItem(100, "Save Preset...");
    menu.addItem(101, "Load Preset...");
    menu.addSeparator();
    menu.addItem(102, "Reset to Default");

    auto options = juce::PopupMenu::Options().withTargetComponent(&presetMenuBtn).withMinimumWidth(presetMenuBtn.getWidth());
    menu.showMenuAsync(options, [this](int result) {
        if (result <= 0) return;
        const auto& factory = GenericNeuralAudioProcessor::getFactoryPresets();
        if (result >= 1 && result <= (int)factory.size()) {
            processorRef.loadPresetByIndex(result - 1);
            updateHeaderDisplay();
        } else if (result == 100) {
            fileChooser = std::make_unique<juce::FileChooser>("Save Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.preset;*.xml");
            fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file != juce::File{}) {
                    if (!file.hasFileExtension("preset;xml")) file = file.withFileExtension(".preset");
                    processorRef.savePresetToFile(file);
                    updateHeaderDisplay();
                }
            });
        } else if (result == 101) {
            fileChooser = std::make_unique<juce::FileChooser>("Load Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.preset;*.xml");
            fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    processorRef.loadPresetFromFile(file);
                    updateHeaderDisplay();
                }
            });
        } else if (result == 102) {
            processorRef.loadPresetByIndex(0);
            updateHeaderDisplay();
        }
    });
}

void GenericNeuralAudioProcessorEditor::showZoomMenu() {
    juce::PopupMenu menu;
    menu.setLookAndFeel(&headerLookAndFeel);
    const int curPct = juce::roundToInt(((float)getWidth() / 520.0f) * 100.0f);
    const int scales[] = { 75, 100, 125, 150 };
    for (int i = 0; i < 4; ++i) {
        menu.addItem(i + 1, juce::String(scales[i]) + "%", true, std::abs(curPct - scales[i]) <= 5);
    }

    auto options = juce::PopupMenu::Options().withTargetComponent(&zoomBtn).withMinimumWidth(zoomBtn.getWidth());
    menu.showMenuAsync(options, [this](int result) {
        if (result == 1) applyZoomFactor(0.75f);
        else if (result == 2) applyZoomFactor(1.00f);
        else if (result == 3) applyZoomFactor(1.25f);
        else if (result == 4) applyZoomFactor(1.50f);
    });
}

void GenericNeuralAudioProcessorEditor::applyZoomFactor(float factor) {
    setSize(juce::roundToInt(520.0f * factor), juce::roundToInt(360.0f * factor));
    updateHeaderDisplay();
}

void GenericNeuralAudioProcessorEditor::updateHeaderDisplay() {
    presetMenuBtn.setButtonText(processorRef.getCurrentPresetName());
    undoBtn.setEnabled(processorRef.undoManager.canUndo());
    redoBtn.setEnabled(processorRef.undoManager.canRedo());

    bool isB = processorRef.isUsingStateB();
    stateABtn.setToggleState(!isB, juce::dontSendNotification);
    stateBBtn.setToggleState(isB,  juce::dontSendNotification);
    copyStateBtn.setButtonText((copyFlashFrames > 0) ? "OK" : (isB ? "B>A" : "A>B"));

    const int pct = juce::roundToInt(((float)getWidth() / 520.0f) * 100.0f);
    zoomBtn.setButtonText(juce::String(pct) + "%");
}

void GenericNeuralAudioProcessorEditor::updateValuePillForSlider(juce::Slider* s) {
    if (s == nullptr) return;
    activePillSlider = s;

    if (s == &driveSlider) {
        pillText = juce::String(juce::roundToInt(driveSlider.getValue() * 100.0)) + "%";
        pillCanvasX = (float)driveSlider.getBounds().getCentreX();
        pillCanvasY = (float)driveSlider.getY() - 14.0f;
    } else if (s == &outputSlider) {
        double db = outputSlider.getValue();
        pillText = (db > 0.05 ? "+" : "") + juce::String(db, 1) + " dB";
        pillCanvasX = (float)outputSlider.getBounds().getCentreX();
        pillCanvasY = (float)outputSlider.getY() - 14.0f;
    }
}

void GenericNeuralAudioProcessorEditor::timerCallback() {
    if (copyFlashFrames > 0) {
        --copyFlashFrames;
        if (copyFlashFrames == 0) updateHeaderDisplay();
    }
    undoBtn.setEnabled(processorRef.undoManager.canUndo());
    redoBtn.setEnabled(processorRef.undoManager.canRedo());

    if (activePillSlider != nullptr && activePillSlider->isMouseButtonDown()) {
        updateValuePillForSlider(activePillSlider);
        pillHoldFrames = 6;
        pillAlpha = 1.0f;
    } else if (pillHoldFrames > 0) {
        --pillHoldFrames;
    } else if (pillAlpha > 0.001f) {
        pillAlpha *= 0.78f;
        if (pillAlpha < 0.01f) {
            pillAlpha = 0.0f;
            activePillSlider = nullptr;
        }
    }

    repaint();
}

void GenericNeuralAudioProcessorEditor::paint(juce::Graphics& g) {
    // main background
    g.fillAll(juce::Colour(0xff181a1f));

    const float headerH = 38.0f;

    // header bar
    g.setColour(juce::Colour(0xff131518));
    g.fillRect(0.0f, 0.0f, (float)getWidth(), headerH);
    g.setColour(juce::Colour(0xff23262d));
    g.fillRect(0.0f, headerH - 1.0f, (float)getWidth(), 1.0f);

    // brand wordmark
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xffe8eaee));
    g.drawText("NEURAL", 16, 0, 60, (int)headerH, juce::Justification::centredLeft, false);
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain)));
    g.setColour(juce::Colour(0xff888d98));
    g.drawText("AUDIO", 76, 0, 60, (int)headerH, juce::Justification::centredLeft, false);

    // header separators
    const float sepH = 18.0f;
    const float sepY = (headerH - sepH) * 0.5f;
    g.setColour(juce::Colour(0xff232730));
    g.fillRect((float)stateABtn.getX() - 8.0f, sepY, 1.0f, sepH);
    g.fillRect((float)zoomBtn.getX() - 8.0f, sepY, 1.0f, sepH);
}

void GenericNeuralAudioProcessorEditor::paintOverChildren(juce::Graphics& g) {
    if (pillAlpha <= 0.01f || pillText.isEmpty()) return;

    const float pillH = 26.0f;
    const juce::Font font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain));
    const float textW = juce::GlyphArrangement::getStringWidth(font, pillText);
    const float pillW = std::max(64.0f, textW + 20.0f);

    juce::Rectangle<float> pill(pillCanvasX - pillW * 0.5f, pillCanvasY - pillH * 0.5f, pillW, pillH);

    g.setColour(juce::Colour(0x40000000).withMultipliedAlpha(pillAlpha));
    g.fillRoundedRectangle(pill.translated(0.0f, 2.0f), 3.0f);

    g.setColour(juce::Colour(0xff242730).withMultipliedAlpha(pillAlpha));
    g.fillRoundedRectangle(pill, 3.0f);

    g.setColour(juce::Colour(0xff3b404d).withMultipliedAlpha(pillAlpha));
    g.drawRoundedRectangle(pill, 3.0f, 1.0f);

    g.setColour(juce::Colour(0xffe2e5eb).withMultipliedAlpha(pillAlpha));
    g.setFont(font);
    g.drawText(pillText, pill, juce::Justification::centred, false);
}

void GenericNeuralAudioProcessorEditor::resized() {
    const int headerH = 38;

    // header layout
    const int btnH = 28;
    const int btnY = (headerH - btnH) / 2;

    presetPrevBtn.setBounds(150, btnY, 28, btnH);
    presetMenuBtn.setBounds(178, btnY, 130, btnH);
    presetNextBtn.setBounds(308, btnY, 28, btnH);

    undoBtn.setBounds(getWidth() - 170, btnY, 28, btnH);
    redoBtn.setBounds(getWidth() - 142, btnY, 28, btnH);

    stateABtn.setBounds(getWidth() - 106, btnY, 24, btnH);
    stateBBtn.setBounds(getWidth() - 82,  btnY, 24, btnH);
    copyStateBtn.setBounds(getWidth() - 58, btnY, 32, btnH);

    zoomBtn.setBounds(getWidth() - 24, btnY, 1, 1); // placed or adjusted as needed
    zoomBtn.setVisible(false); // optional compact view

    // knob layout
    const int contentY = headerH + 20;
    const int knobSize = 110;
    const int spacing = 160;
    const int startX = (getWidth() - spacing) / 2;

    driveSlider.setBounds(startX - knobSize / 2, contentY + 20, knobSize, knobSize);
    driveLabel.setBounds(driveSlider.getX(), driveSlider.getBottom() + 4, knobSize, 22);

    outputSlider.setBounds(startX + spacing - knobSize / 2, contentY + 20, knobSize, knobSize);
    outputLabel.setBounds(outputSlider.getX(), outputSlider.getBottom() + 4, knobSize, 22);
}
