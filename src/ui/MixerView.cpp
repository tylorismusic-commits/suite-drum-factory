/*
    MixerView.cpp - Implementation of per-mic drum mixer console and DSP rack.
*/

#include "MixerView.h"

namespace glorp
{

namespace
{
juce::Colour getMicColor(const juce::String &name)
{
    const auto n = name.toLowerCase();
    if (n.contains("kick") || n.contains("kdrum"))
        return juce::Colour(0xffe67e22); // Orange
    if (n.contains("snare"))
        return juce::Colour(0xff2ecc71); // Emerald Green
    if (n.contains("hihat") || n.contains("hh"))
        return juce::Colour(0xfff1c40f); // Amber / Yellow
    if (n.contains("tom"))
        return juce::Colour(0xff00d2d3); // Cyan
    if (n.contains("ride") || n.contains("crash") || n.contains("cymbal") || n.contains("splash")
        || n.contains("china"))
        return juce::Colour(0xffff4757); // Vibrant Magenta
    if (n.contains("overhead") || n.contains("oh"))
        return juce::Colour(0xff3498db); // Sky Blue
    if (n.contains("room") || n.contains("amb"))
        return juce::Colour(0xff9b59b6); // Purple
    return juce::Colour(0xff1abc9c);     // Teal default
}

juce::Colour insertColour(InsertType type)
{
    switch (type)
    {
        case InsertType::eq: return juce::Colour(0xff3fa7ff);
        case InsertType::compressor: return juce::Colour(0xff52d273);
        case InsertType::drive: return juce::Colour(0xffe67e22);
        case InsertType::transient: return juce::Colour(0xffb07cff);
        case InsertType::limiter: return juce::Colour(0xfff1c40f);
        case InsertType::clipper: return juce::Colour(0xffe74c3c);
        default: return juce::Colour(0xff6b6b72);
    }
}

juce::Colour eqBandColour(int band)
{
    static const juce::Colour colours[eqBandCount] {
        juce::Colour(0xff7fb3ff), juce::Colour(0xff52d273), juce::Colour(0xfff1c40f),
        juce::Colour(0xffe67e22), juce::Colour(0xffff6b81), juce::Colour(0xffb07cff),
        juce::Colour(0xff00d2d3), juce::Colour(0xffe74c3c)};
    return colours[static_cast<size_t>(juce::jlimit(0, eqBandCount - 1, band))];
}

const char *eqBandShortName(int band)
{
    static const char *names[eqBandCount] {"LC", "LS", "1", "2", "3", "4", "HS", "HC"};
    return names[static_cast<size_t>(juce::jlimit(0, eqBandCount - 1, band))];
}

const char *eqBandLongName(int band)
{
    static const char *names[eqBandCount] {"LOW CUT", "LOW SHELF", "BELL 1", "BELL 2",
                                           "BELL 3", "BELL 4", "HIGH SHELF", "HIGH CUT"};
    return names[static_cast<size_t>(juce::jlimit(0, eqBandCount - 1, band))];
}

class InsertTypePicker : public juce::Component
{
  public:
    InsertTypePicker(const Theme &themeToUse, std::function<void(InsertType)> callback)
        : theme(themeToUse), onPick(std::move(callback))
    {
        setSize(196, 6 * 30 + 16);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics &g) override
    {
        g.setColour(theme.panel);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
        g.setColour(theme.panelEdge);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 1.2f);

        for (int i = 0; i < 6; ++i)
        {
            const auto r = row(i);
            if (i == hovered)
            {
                g.setColour(theme.accent.withAlpha(0.16f));
                g.fillRoundedRectangle(r.reduced(4.0f, 1.0f), 4.0f);
            }
            const auto type = typeAt(i);
            g.setColour(insertColour(type));
            g.fillEllipse(r.getX() + 10.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
            g.setColour(theme.ink);
            g.setFont(uiFont(10.5f));
            g.drawText(typeName(type), r.withTrimmedLeft(26.0f).toNearestInt(),
                       juce::Justification::centredLeft, false);
        }
    }

    void mouseMove(const juce::MouseEvent &e) override
    {
        hovered = rowAt(e.position);
        repaint();
    }
    void mouseExit(const juce::MouseEvent &) override
    {
        hovered = -1;
        repaint();
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        const int index = rowAt(e.position);
        if (index < 0)
            return;
        if (onPick)
            onPick(typeAt(index));
    }

  private:
    Theme theme;
    std::function<void(InsertType)> onPick;
    int hovered = -1;

    static InsertType typeAt(int index)
    {
        switch (index)
        {
            case 0: return InsertType::eq;
            case 1: return InsertType::compressor;
            case 2: return InsertType::drive;
            case 3: return InsertType::transient;
            case 4: return InsertType::limiter;
            case 5: return InsertType::clipper;
            default: return InsertType::none;
        }
    }

    static juce::String typeName(InsertType type)
    {
        if (type == InsertType::eq) return "Graphical EQ";
        if (type == InsertType::compressor) return "Compressor";
        if (type == InsertType::drive) return "Drive";
        if (type == InsertType::transient) return "Transient Shaper";
        if (type == InsertType::limiter) return "Limiter";
        if (type == InsertType::clipper) return "Clipper";
        return "Off";
    }

    juce::Rectangle<float> row(int index) const
    {
        return {4.0f, 8.0f + index * 30.0f, static_cast<float>(getWidth() - 8), 28.0f};
    }

    int rowAt(juce::Point<float> pos) const
    {
        for (int i = 0; i < 6; ++i)
            if (row(i).contains(pos))
                return i;
        return -1;
    }
};

// Full-window transparent layer that closes the insert picker when clicking
// anywhere outside the picker itself.
class InsertPickerLayer : public juce::Component
{
  public:
    explicit InsertPickerLayer(std::function<void()> dismissCallback)
        : onDismiss(std::move(dismissCallback))
    {
        setInterceptsMouseClicks(true, true);
    }

    void paint(juce::Graphics &) override {}

    void mouseDown(const juce::MouseEvent &) override
    {
        if (onDismiss != nullptr)
        {
            const auto callback = onDismiss;
            juce::MessageManager::callAsync([callback]
                                            {
                                                if (callback != nullptr)
                                                    callback();
                                            });
        }
    }

  private:
    std::function<void()> onDismiss;
};

float linearToDb(float linear) noexcept
{
    return linear > 1.0e-5f ? 20.0f * std::log10(linear) : -100.0f;
}

float dbToNormalized(float db) noexcept
{
    // Maps -60dB .. +6dB to 0.0 .. 1.0
    if (db <= -60.0f)
        return 0.0f;
    if (db >= 6.0f)
        return 1.0f;
    return (db + 60.0f) / 66.0f;
}

float normalizedToGain(float norm) noexcept
{
    // 0.0 = -inf (0.0), 0.75 = 0dB (1.0), 1.0 = +6dB (2.0)
    if (norm <= 0.01f)
        return 0.0f;
    if (norm <= 0.75f)
    {
        const float t = norm / 0.75f;
        return std::pow(t, 2.5f);
    }
    const float t = (norm - 0.75f) / 0.25f;
    return 1.0f + t * 1.0f;
}

float gainToNormalized(float gain) noexcept
{
    if (gain <= 0.001f)
        return 0.0f;
    if (gain <= 1.0f)
        return std::pow(gain, 1.0f / 2.5f) * 0.75f;
    return 0.75f + std::clamp((gain - 1.0f), 0.0f, 1.0f) * 0.25f;
}

juce::String formatDb(float gain)
{
    if (gain <= 0.001f)
        return "-inf";
    const float db = linearToDb(gain);
    if (std::abs(db) < 0.05f)
        return "0.0 dB";
    return (db > 0.0f ? "+" : "") + juce::String(db, 1) + " dB";
}
} // namespace

//==============================================================================
// HandButton
//==============================================================================

MixerView::HandButton::HandButton(const juce::String &name, bool isToggle) : juce::Button(name), isToggle_(isToggle)
{
}

void MixerView::HandButton::paintButton(juce::Graphics &g, bool isHover, bool isDown)
{
    const auto r = getLocalBounds().toFloat().reduced(1.0f);
    const auto ink = theme.ink;

    // Comic offset drop shadow (light non-minimal mode only)
    if (!theme.isDark && !theme.minimalStyle)
    {
        g.setColour(ink.withAlpha(0.35f));
        g.fillRoundedRectangle(r.translated(2.0f, 2.0f), 4.0f);
    }

    juce::Colour bg;
    if (active)
        bg = activeColour;
    else if (isDown)
        bg = theme.isDark ? juce::Colour(0xff282a30) : juce::Colour(0xffe5e9dc);
    else if (isHover)
        bg = theme.isDark ? juce::Colour(0xff24252a) : juce::Colour(0xfff4f6ee);
    else
        bg = theme.panel;

    g.setColour(bg);
    g.fillRoundedRectangle(r, 4.0f);

    const auto borderCol = isHover ? theme.accent : theme.panelEdge;
    drawSketchRect(g, r, borderCol, active ? 2.0f : 1.4f);

    juce::Colour textCol = theme.ink;

    if (active)
    {
        // Amber and other bright active fills need dark text; deep red/black
        // active fills keep white text.
        textCol = activeColour.getPerceivedBrightness() > 0.55f ? theme.background : juce::Colours::white;
    }

    g.setColour(textCol);
    g.setFont(displayFont(std::min(r.getHeight() * 0.58f, 13.0f)));
    g.drawText(getButtonText(), r, juce::Justification::centred, false);
}

//==============================================================================
// ChannelStripComponent
//==============================================================================

MixerView::ChannelStripComponent::ChannelStripComponent(MixerView &o, PluginProcessor &proc, int chIdx)
    : owner(o), processor(proc), channelIndex_(chIdx)
{
    phaseBtn.setButtonText(juce::CharPointer_UTF8("\xc3\x98")); // Ø
    phaseBtn.activeColour = juce::Colour(0xff00ffcc);           // Neon Cyan
    phaseBtn.onClick = [this]()
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.phaseInvert = !params.phaseInvert;
        processor.getSampler().setMicStripParams(channelIndex_, params);
        phaseBtn.setActive(params.phaseInvert);
    };
    addAndMakeVisible(phaseBtn);

    fxBtn.setButtonText("FX");
    fxBtn.activeColour = juce::Colour(0xff4cc2a8);
    fxBtn.onClick = [this]()
    {
        owner.closePopups();
        if (owner.insertRackPopup != nullptr)
            owner.insertRackPopup->open(false, channelIndex_);
    };
    addAndMakeVisible(fxBtn);

    bleedBtn.setButtonText("BLD");
    bleedBtn.activeColour = juce::Colour(0xff4cc2a8);
    bleedBtn.onClick = [this]() { owner.openBleedMatrix(channelIndex_); };
    addAndMakeVisible(bleedBtn);

    soloBtn.setButtonText("S");
    soloBtn.activeColour = juce::Colour(0xff4cc2a8);
    soloBtn.onClick = [this]()
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.solo = !params.solo;
        processor.getSampler().setMicStripParams(channelIndex_, params);
        soloBtn.setActive(params.solo);
    };
    addAndMakeVisible(soloBtn);

    muteBtn.setButtonText("M");
    muteBtn.activeColour = juce::Colour(0xffe74c3c);
    muteBtn.onClick = [this]()
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.mute = !params.mute;
        processor.getSampler().setMicStripParams(channelIndex_, params);
        muteBtn.setActive(params.mute);
    };
    addAndMakeVisible(muteBtn);

    monoBtn.setButtonText("MONO");
    monoBtn.activeColour = juce::Colour(0xffa78bfa);
    monoBtn.onClick = [this]()
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.mono = !params.mono;

        if (params.mono)
            params.pan = 0.0f;

        processor.getSampler().setMicStripParams(channelIndex_, params);
        updateFromState();
        repaint();
    };
    addAndMakeVisible(monoBtn);

    busBtn.onClick = [this]()
    {
        juce::PopupMenu menu;
        const int currentBus = processor.getSampler().getMicOutputBus(channelIndex_);
        for (int b = 0; b < config::MAX_OUTPUT_BUSES; ++b)
        {
            const juce::String busLabel = (b == 0)
                                              ? "Internal Sums (Kick/Snare/Toms/Cymbals/Room)"
                                              : ("Direct Out " + juce::String(b * 2 + 1) + "/"
                                                 + juce::String(b * 2 + 2));
            menu.addItem(busLabel,
                         true,
                         b == currentBus,
                         [this, b]()
                         {
                             processor.setMicOutputBus(channelIndex_, b);
                             updateFromState();
                         });
        }
        juce::PopupMenu::Options opts;
        opts = opts.withTargetComponent(&busBtn).withParentComponent(&owner);
        menu.showMenuAsync(opts);
    };
    addAndMakeVisible(busBtn);

    const bool externalKit = !processor.getSampler().isFactoryPieceMode();
    phaseBtn.setVisible(externalKit);
    fxBtn.setVisible(false);
    bleedBtn.setVisible(false);
    monoBtn.setVisible(externalKit);
    busBtn.setVisible(true);

    bleedKnob = std::make_unique<Knob>("BLEED", 0.0f, 1.0f, 0.15f);
    bleedKnob->setTheme(theme);
    bleedKnob->onValueChanged = [this](float v)
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.bleed = juce::jlimit(0.0f, 1.0f, v);
        processor.getSampler().setMicStripParams(channelIndex_, params);
    };
    addAndMakeVisible(*bleedKnob);
    bleedKnob->setVisible(!externalKit);

    updateFromState();
    setTheme(owner.theme);
}

void MixerView::ChannelStripComponent::updateFromState()
{
    const auto params = processor.getSampler().getMicStripParams(channelIndex_);
    const bool externalKit = !processor.getSampler().isFactoryPieceMode();
    phaseBtn.setVisible(externalKit);
    monoBtn.setVisible(externalKit);
    if (bleedKnob != nullptr)
        bleedKnob->setVisible(!externalKit);

    phaseBtn.setActive(params.phaseInvert);
    soloBtn.setActive(params.solo);
    muteBtn.setActive(params.mute);
    monoBtn.setActive(params.mono);
    const bool fxActive = params.compEnabled || params.saturationDrive > 0.05f
                          || std::abs(params.transientAttack) > 0.05f || std::abs(params.eqLowGainDb) > 0.5f
                          || std::abs(params.eqMidGainDb) > 0.5f || std::abs(params.eqHighGainDb) > 0.5f;
    fxBtn.setActive(fxActive);

    if (bleedKnob != nullptr)
        bleedKnob->setValue(params.bleed);

    const auto &bm = processor.getSampler().getBleedMatrix();
    bool hasBleed = false;
    for (int p = 0; p < BleedMatrix::maxPieces; ++p)
    {
        if (bm.getCellGain(p, channelIndex_) > 0.05f && !bm.isCellMuted(p, channelIndex_))
        {
            hasBleed = true;
            break;
        }
    }
    bleedBtn.setActive(hasBleed && bm.getMode() != BleedMatrix::Mode::disabled);

    const int b = processor.getSampler().getMicOutputBus(channelIndex_);
    busBtn.setButtonText(b == 0 ? "SUMS" : ("OUT " + juce::String(b * 2 + 1) + "/" + juce::String(b * 2 + 2)));
    busBtn.activeColour = (b == 0) ? juce::Colour(0xff4cc2a8) : juce::Colour(0xff3498db);
    busBtn.setActive(b > 0);
    busBtn.setActive(b > 0);

    repaint();
}

void MixerView::ChannelStripComponent::resized()
{
    const auto r = getLocalBounds().toFloat();
    const bool isCompact = (r.getHeight() < 340.0f);

    const float rowH = isCompact ? 17.0f : 18.0f;
    for (int i = 0; i < InsertChain::slotCount; ++i)
        insertBounds[static_cast<size_t>(i)] = {r.getX() + 5.0f, 32.0f + i * (rowH + 2.0f),
                                                r.getWidth() - 10.0f, rowH};

    const float insertBottom = 32.0f + InsertChain::slotCount * (rowH + 2.0f);
    const float panY = insertBottom + 4.0f;
    panBarBounds = juce::Rectangle<float>(r.getX() + 8.0f, panY, r.getWidth() - 54.0f, isCompact ? 14.0f : 16.0f);

    const bool externalKit = !processor.getSampler().isFactoryPieceMode();
    if (bleedKnob != nullptr)
    {
        if (externalKit)
            bleedKnob->setBounds(0, 0, 0, 0);
        else
            bleedKnob->setBounds(static_cast<int>(r.getRight() - 46.0f), static_cast<int>(panY - 8.0f), 40, 54);
    }

    if (externalKit)
    {
        phaseBtn.setBounds(static_cast<int>(r.getRight() - 46.0f), static_cast<int>(panY - 8.0f), 40, 22);
        monoBtn.setBounds(static_cast<int>(r.getRight() - 46.0f), static_cast<int>(panY + 17.0f), 40, 22);
    }
    else
    {
        phaseBtn.setBounds(0, 0, 0, 0);
        monoBtn.setBounds(0, 0, 0, 0);
    }

    const float soloMuteY = panY + 46.0f;
    const float soloMuteH = isCompact ? 18.0f : 20.0f;
    soloBtn.setBounds(r.getX() + 6.0f, soloMuteY, (r.getWidth() - 16.0f) * 0.48f, soloMuteH);
    muteBtn.setBounds(r.getX() + 8.0f + (r.getWidth() - 16.0f) * 0.48f, soloMuteY,
                      (r.getWidth() - 16.0f) * 0.48f, soloMuteH);

    const float faderTop = soloMuteY + soloMuteH + 8.0f;
    const float faderBottom = isCompact ? (r.getBottom() - 44.0f) : (r.getBottom() - 54.0f);
    const float faderH = std::max(30.0f, faderBottom - faderTop);

    faderTrackBounds = juce::Rectangle<float>(r.getX() + 14.0f, faderTop, 24.0f, faderH);
    meterBounds = juce::Rectangle<float>(r.getX() + 46.0f, faderTop, 16.0f, faderH);

    const auto params = processor.getSampler().getMicStripParams(channelIndex_);
    const float norm = gainToNormalized(params.level);
    const float thumbY = faderTrackBounds.getBottom() - norm * faderTrackBounds.getHeight()
                         - (isCompact ? 8.0f : 10.0f);
    faderThumbBounds = juce::Rectangle<float>(faderTrackBounds.getX() - 4.0f, thumbY,
                                              faderTrackBounds.getWidth() + 8.0f, isCompact ? 16.0f : 20.0f);

    fxBtn.setBounds(0, 0, 0, 0);
    bleedBtn.setBounds(0, 0, 0, 0);
    busBtn.setBounds(r.getX() + 6.0f, r.getBottom() - 23.0f, r.getWidth() - 12.0f, 18.0f);
}

void MixerView::ChannelStripComponent::setTheme(const Theme &t)
{
    theme = t;
    phaseBtn.setTheme(t);
    fxBtn.setTheme(t);
    bleedBtn.setTheme(t);
    soloBtn.setTheme(t);
    muteBtn.setTheme(t);
    monoBtn.setTheme(t);
    busBtn.setTheme(t);
    if (bleedKnob != nullptr)
        bleedKnob->setTheme(t);
    repaint();
}

void MixerView::ChannelStripComponent::paint(juce::Graphics &g)
{
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    const auto name = processor.getSampler().micStripTitle(channelIndex_);
    const auto accent = getMicColor(name);
    const auto params = processor.getSampler().getMicStripParams(channelIndex_);
    const auto ink = theme.ink;

    // Comic offset drop shadow (light non-minimal mode only)
    if (!theme.isDark && !theme.minimalStyle)
    {
        g.setColour(ink.withAlpha(0.32f));
        g.fillRoundedRectangle(r.translated(3.0f, 3.0f), 6.0f);
    }

    // Strip background
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 6.0f);

    // Sketched ink border
    drawSketchRect(g, r, theme.panelEdge, 1.8f, static_cast<uint32_t>(channelIndex_ * 7 + 13));

    // Top Channel Badge
    auto badgeR = juce::Rectangle<float>(r.getX() + 4.0f, r.getY() + 4.0f, r.getWidth() - 8.0f, 26.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(badgeR, 4.0f);
    drawSketchRect(
        g, badgeR, theme.isDark ? theme.panelEdge : ink, 1.5f, static_cast<uint32_t>(channelIndex_ * 11 + 3));

    g.setColour(juce::Colours::white);
    g.setFont(displayFont(11.0f));

    auto badgeText = badgeR.reduced(2.0f).toNearestInt();
    auto displayName = name.toUpperCase();

    if (name.containsIgnoreCase("layer"))
    {
        displayName = name.upToFirstOccurrenceOf(" layer", false, true).toUpperCase();

        const juce::Rectangle<float> tag(badgeR.getRight() - 33.0f,
                                         badgeR.getCentreY() - 7.0f,
                                         29.0f,
                                         14.0f);
        g.setColour(theme.background.withAlpha(0.62f));
        g.fillRoundedRectangle(tag, 3.0f);
        g.setFont(uiFont(7.5f).boldened());
        g.setColour(juce::Colours::white);
        g.drawText("LAYER", tag.toNearestInt(), juce::Justification::centred, false);

        badgeText = badgeText.withTrimmedRight(31);
        g.setFont(displayFont(10.0f));
    }

    g.drawFittedText(displayName, badgeText, juce::Justification::centred, 1);

    // Insert rows
    for (int slot = 0; slot < InsertChain::slotCount; ++slot)
    {
        auto area = insertBounds[static_cast<size_t>(slot)];
        if (area.isEmpty())
            continue;
        const auto state = processor.getSampler().getInsertState(false, channelIndex_, slot);
        g.setColour(theme.trough);
        g.fillRoundedRectangle(area, 3.0f);
        g.setColour(theme.panelEdge);
        g.drawRoundedRectangle(area, 3.0f, 1.0f);

        if (state.type == InsertType::none)
        {
            g.setColour(theme.isDark ? theme.inkSoft : theme.ink.withAlpha(0.62f));
            g.setFont(uiFont(8.0f).boldened());
            g.drawText("+ ADD INSERT", area.reduced(5.0f, 0.0f), juce::Justification::centredLeft, false);
        }
        else
        {
            const auto colour = insertColour(state.type);
            g.setColour(colour.withAlpha(state.bypass ? 0.25f : 0.85f));
            g.fillRoundedRectangle(area.withWidth(3.0f).reduced(0.0f, 1.0f), 2.0f);

            g.setColour(state.bypass ? theme.inkSoft.withAlpha(0.5f) : theme.ink);
            g.setFont(uiFont(8.0f));
            g.drawFittedText(juce::String(insertTypeName(state.type)),
                             area.withTrimmedLeft(18.0f).withTrimmedRight(16.0f).toNearestInt(),
                             juce::Justification::centredLeft, 1, 0.7f);

            g.setColour(state.bypass ? theme.inkSoft.withAlpha(0.5f) : colour);
            g.fillEllipse(area.getX() + 6.0f, area.getCentreY() - 3.0f, 6.0f, 6.0f);
            g.setColour(theme.inkSoft);
            g.drawText("x", area.getRight() - 14.0f, area.getY(), 12.0f, area.getHeight(),
                       juce::Justification::centred, false);
        }
    }

    // Pan Bar: trough slot with border
    g.setColour(theme.trough);
    g.fillRoundedRectangle(panBarBounds, 3.0f);
    drawSketchRect(g, panBarBounds, theme.panelEdge, 1.2f, 99);

    const float centerX = panBarBounds.getCentreX();
    const float panOffset = params.pan * (panBarBounds.getWidth() * 0.45f);
    if (std::abs(params.pan) > 0.02f)
    {
        g.setColour(theme.accent);
        if (params.pan > 0.0f)
            g.fillRect(centerX, panBarBounds.getY() + 2.0f, panOffset, panBarBounds.getHeight() - 4.0f);
        else
            g.fillRect(centerX + panOffset, panBarBounds.getY() + 2.0f, -panOffset, panBarBounds.getHeight() - 4.0f);
    }
    // Center tick
    g.setColour(theme.ink);
    g.drawVerticalLine(static_cast<int>(centerX), panBarBounds.getY(), panBarBounds.getBottom());

    g.setColour(theme.ink);
    g.setFont(displayFont(9.0f));
    juce::String panStr = "C";
    if (params.pan < -0.05f)
        panStr = "L" + juce::String(static_cast<int>(-params.pan * 100.0f));
    else if (params.pan > 0.05f)
        panStr = "R" + juce::String(static_cast<int>(params.pan * 100.0f));
    g.drawText(panStr, panBarBounds, juce::Justification::centred, false);

    // Fader Track: sketched vertical slot
    g.setColour(theme.trough);
    g.fillRoundedRectangle(faderTrackBounds, 4.0f);
    drawSketchRect(g, faderTrackBounds, theme.panelEdge, 1.4f, 101);

    // 0dB Tick mark
    const float zeroY = faderTrackBounds.getBottom() - 0.75f * faderTrackBounds.getHeight();
    g.setColour(theme.ink.withAlpha(0.6f));
    g.drawHorizontalLine(static_cast<int>(zeroY), faderTrackBounds.getX() - 4.0f, faderTrackBounds.getX());

    // LED Meter: retro comic meter capsule
    g.setColour(theme.trough);
    g.fillRoundedRectangle(meterBounds, 3.0f);
    drawSketchRect(g, meterBounds, theme.panelEdge, 1.3f, 202);

    const float peakL = processor.getSampler().getMicPeakL(channelIndex_);
    const float peakR = processor.getSampler().getMicPeakR(channelIndex_);
    const float rmsL = processor.getSampler().getMicRmsL(channelIndex_);
    const float rmsR = processor.getSampler().getMicRmsR(channelIndex_);
    const float normPeakL = dbToNormalized(linearToDb(peakL));
    const float normPeakR = dbToNormalized(linearToDb(peakR));
    const float normRmsL = dbToNormalized(linearToDb(rmsL));
    const float normRmsR = dbToNormalized(linearToDb(rmsR));

    const float meterH = meterBounds.getHeight() - 4.0f;
    const float meterW = meterBounds.getWidth() - 4.0f;
    const float meterX = meterBounds.getX() + 2.0f;
    const float meterY = meterBounds.getY() + 2.0f;
    const float gap = 2.0f;
    const float sideW = std::max(2.0f, (meterW - gap) * 0.5f);

    // Stereo channel meter: L and R get their own strips, so hard-panned or
    // right-only mic channels still show level.
    for (int side = 0; side < 2; ++side)
    {
        const float sideX = meterX + static_cast<float>(side) * (sideW + gap);
        const float normRms = side == 0 ? normRmsL : normRmsR;
        const float normPeak = side == 0 ? normPeakL : normPeakR;

        if (normRms > 0.01f)
        {
            constexpr int numSegs = 16;
            const int activeSegs = std::clamp(static_cast<int>(normRms * numSegs), 1, numSegs);
            const float segH = meterH / numSegs;

            for (int s = 0; s < activeSegs; ++s)
            {
                const float sy = meterY + meterH - (s + 1) * segH;
                juce::Colour segCol = juce::Colour(0xff2ecc71); // Green
                if (s >= 13)
                    segCol = juce::Colour(0xffe74c3c); // Red peak
                else if (s >= 10)
                    segCol = juce::Colour(0xfff1c40f); // Amber / Yellow

                g.setColour(segCol);
                g.fillRoundedRectangle(sideX, sy + 0.5f, sideW, segH - 1.0f, 1.0f);
            }
        }

        if (normPeak > 0.02f)
        {
            const float peakY = meterY + meterH - normPeak * meterH;
            g.setColour(normPeak >= 0.98f ? juce::Colour(0xffe74c3c) : theme.ink);
            g.fillRect(sideX, peakY - 1.0f, sideW, 2.0f);
        }
    }

    // Fader Thumb: hand-drawn beveled paper cap with ink border & drop shadow
    const float normGain = gainToNormalized(params.level);
    const float thumbY = faderTrackBounds.getBottom() - normGain * faderTrackBounds.getHeight() - 10.0f;
    auto thumbR =
        juce::Rectangle<float>(faderTrackBounds.getX() - 5.0f, thumbY, faderTrackBounds.getWidth() + 10.0f, 20.0f);

    // Drop shadow
    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.4f));
        g.fillRoundedRectangle(thumbR.translated(2.0f, 2.0f), 3.0f);
    }

    // Thumb cap
    g.setColour(theme.panel);
    g.fillRoundedRectangle(thumbR, 3.0f);
    drawSketchRect(g, thumbR, theme.panelEdge, 1.8f, 55);

    // Center indicator stripe
    g.setColour(accent);
    g.fillRect(thumbR.getX() + 2.0f, thumbR.getCentreY() - 1.5f, thumbR.getWidth() - 4.0f, 3.0f);

    // Numeric dB Display
    const bool isCompact = (r.getHeight() < 300.0f);
    const float dbY = isCompact ? (r.getBottom() - 41.0f) : (r.getBottom() - 50.0f);
    auto dbRect = juce::Rectangle<float>(r.getX() + 4.0f, dbY, r.getWidth() - 8.0f, isCompact ? 16.0f : 18.0f);
    g.setColour(theme.trough);
    g.fillRoundedRectangle(dbRect, 3.0f);
    drawSketchRect(g, dbRect, theme.panelEdge, 1.2f, 77);

    g.setColour(params.level > 1.0f ? juce::Colour(0xffd03020) : theme.ink);
    g.setFont(displayFont(isCompact ? 8.5f : 9.5f));
    g.drawText(formatDb(params.level), dbRect, juce::Justification::centred, false);
}

void MixerView::ChannelStripComponent::mouseDown(const juce::MouseEvent &e)
{
    const auto p = e.position;
    for (int slot = 0; slot < InsertChain::slotCount; ++slot)
    {
        const auto area = insertBounds[static_cast<size_t>(slot)];
        if (!area.contains(p))
            continue;

        const auto state = processor.getSampler().getInsertState(false, channelIndex_, slot);
        if (state.type == InsertType::none)
        {
            owner.showInsertMenu(false, channelIndex_, slot, this, area.toNearestInt());
            return;
        }
        if (p.x <= area.getX() + 15.0f)
        {
            processor.getSampler().setInsertBypass(false, channelIndex_, slot, !state.bypass);
            repaint();
            return;
        }
        if (p.x >= area.getRight() - 16.0f)
        {
            processor.getSampler().setInsertType(false, channelIndex_, slot, InsertType::none);
            repaint();
            return;
        }
        owner.openInsertRack(false, channelIndex_, slot);
        return;
    }

    if (panBarBounds.contains(p))
    {
        draggingPan = true;
        mouseDrag(e);
    }
    else if (p.y >= faderTrackBounds.getY() - 10.0f && p.y <= faderTrackBounds.getBottom() + 10.0f)
    {
        draggingFader = true;
        mouseDrag(e);
    }
}

void MixerView::ChannelStripComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (draggingPan)
    {
        const float relX = (e.position.x - panBarBounds.getX()) / panBarBounds.getWidth();
        const float newPan = std::clamp(relX * 2.0f - 1.0f, -1.0f, 1.0f);
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.pan = newPan;
        processor.getSampler().setMicStripParams(channelIndex_, params);
        repaint();
    }
    else if (draggingFader)
    {
        const float norm =
            std::clamp((faderTrackBounds.getBottom() - e.position.y) / faderTrackBounds.getHeight(), 0.0f, 1.0f);
        const float newLevel = normalizedToGain(norm);
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.level = newLevel;
        processor.getSampler().setMicStripParams(channelIndex_, params);
        repaint();
    }
}

void MixerView::ChannelStripComponent::mouseDoubleClick(const juce::MouseEvent &e)
{
    if (panBarBounds.contains(e.position))
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.pan = 0.0f;
        processor.getSampler().setMicStripParams(channelIndex_, params);
        repaint();
    }
    else if (faderTrackBounds.expanded(10.0f, 0.0f).contains(e.position))
    {
        auto params = processor.getSampler().getMicStripParams(channelIndex_);
        params.level = 1.0f; // 0dB
        processor.getSampler().setMicStripParams(channelIndex_, params);
        repaint();
    }
}

//==============================================================================
// MasterStripComponent
//==============================================================================

MixerView::BusStripComponent::BusStripComponent(MixerView &o, PluginProcessor &p, int busIndex)
    : owner(o), processor(p), busIndex_(busIndex)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    soloBtn.onClick = [this]
    {
        auto params = processor.getSampler().getFactoryBusParams(busIndex_);
        params.solo = !params.solo;
        processor.getSampler().setFactoryBusParams(busIndex_, params);
        soloBtn.setActive(params.solo);
        repaint();
    };
    muteBtn.onClick = [this]
    {
        auto params = processor.getSampler().getFactoryBusParams(busIndex_);
        params.mute = !params.mute;
        processor.getSampler().setFactoryBusParams(busIndex_, params);
        muteBtn.setActive(params.mute);
        repaint();
    };
    fxBtn.setButtonText("FX");
    fxBtn.activeColour = juce::Colour(0xff4cc2a8);
    fxBtn.onClick = [this]
    {
        owner.closePopups();
        if (owner.insertRackPopup != nullptr)
            owner.insertRackPopup->open(true, busIndex_);
    };
    addAndMakeVisible(soloBtn);
    addAndMakeVisible(muteBtn);
    fxBtn.setVisible(false);
}

void MixerView::BusStripComponent::setTheme(const Theme &t)
{
    theme = t;
    soloBtn.setTheme(t);
    muteBtn.setTheme(t);
    fxBtn.setTheme(t);
    repaint();
}

void MixerView::BusStripComponent::resized()
{
    const auto r = getLocalBounds().toFloat();
    const bool isCompact = (r.getHeight() < 340.0f);

    const float rowH = isCompact ? 18.0f : 19.0f;
    for (int i = 0; i < InsertChain::slotCount; ++i)
        insertBounds[static_cast<size_t>(i)] = {r.getX() + 5.0f, 30.0f + i * (rowH + 2.0f),
                                                r.getWidth() - 10.0f, rowH};

    const float panY = 30.0f + InsertChain::slotCount * (rowH + 2.0f) + 4.0f;
    panBarBounds = juce::Rectangle<float>(r.getX() + 8.0f, panY, r.getWidth() - 16.0f, isCompact ? 15.0f : 18.0f);

    const float soloMuteY = panY + 28.0f;
    const float soloMuteH = isCompact ? 19.0f : 22.0f;
    soloBtn.setBounds(r.getX() + 6.0f, soloMuteY, (r.getWidth() - 16.0f) * 0.48f, soloMuteH);
    muteBtn.setBounds(r.getX() + 8.0f + (r.getWidth() - 16.0f) * 0.48f, soloMuteY,
                      (r.getWidth() - 16.0f) * 0.48f, soloMuteH);

    const float faderTop = soloMuteY + soloMuteH + 7.0f;
    const float faderBottom = isCompact ? (r.getBottom() - 44.0f) : (r.getBottom() - 56.0f);
    const float faderH = std::max(30.0f, faderBottom - faderTop);

    faderTrackBounds = juce::Rectangle<float>(r.getX() + 14.0f, faderTop, 24.0f, faderH);
    meterBounds = juce::Rectangle<float>(r.getX() + 46.0f, faderTop, 16.0f, faderH);

    const auto params = processor.getSampler().getFactoryBusParams(busIndex_);
    const float norm = gainToNormalized(params.level);
    const float thumbY = faderTrackBounds.getBottom() - norm * faderTrackBounds.getHeight()
                         - (isCompact ? 8.0f : 10.0f);
    faderThumbBounds = juce::Rectangle<float>(faderTrackBounds.getX() - 4.0f, thumbY,
                                              faderTrackBounds.getWidth() + 8.0f, isCompact ? 16.0f : 20.0f);
    fxBtn.setBounds(0, 0, 0, 0);
}

void MixerView::BusStripComponent::paint(juce::Graphics &g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(theme.panel);
    g.fillRoundedRectangle(b, 7.0f);
    drawSketchRect(g, b, theme.panelEdge, 1.2f, 301);

    const auto &sampler = processor.getSampler();
    const auto params = sampler.getFactoryBusParams(busIndex_);
    soloBtn.setActive(params.solo);
    muteBtn.setActive(params.mute);
    const auto title = sampler.factoryBusTitle(busIndex_);
    juce::Colour colour = juce::Colour(0xff9b59b6);
    switch (busIndex_)
    {
        case 0: colour = juce::Colour(0xffe67e22); break;
        case 1: colour = juce::Colour(0xff2ecc71); break;
        case 2: colour = juce::Colour(0xff00d2d3); break;
        case 3: colour = juce::Colour(0xffff4757); break;
        default: colour = juce::Colour(0xff9b59b6); break;
    }

    auto header = b.removeFromTop(26.0f).reduced(5.0f, 3.0f);
    g.setColour(colour.withAlpha(0.22f));
    g.fillRoundedRectangle(header, 4.0f);
    g.setColour(colour);
    g.setFont(displayFont(10.0f));
    g.drawFittedText(title.toUpperCase(), header.toNearestInt(), juce::Justification::centred, 1, 0.7f);

    for (int slot = 0; slot < InsertChain::slotCount; ++slot)
    {
        auto area = insertBounds[static_cast<size_t>(slot)];
        if (area.isEmpty())
            continue;
        const auto state = processor.getSampler().getInsertState(true, busIndex_, slot);
        g.setColour(theme.trough);
        g.fillRoundedRectangle(area, 3.0f);
        g.setColour(theme.panelEdge);
        g.drawRoundedRectangle(area, 3.0f, 1.0f);

        if (state.type == InsertType::none)
        {
            g.setColour(theme.isDark ? theme.inkSoft : theme.ink.withAlpha(0.62f));
            g.setFont(uiFont(8.0f).boldened());
            g.drawText("+ ADD INSERT", area.reduced(5.0f, 0.0f), juce::Justification::centredLeft, false);
        }
        else
        {
            const auto typeColour = insertColour(state.type);
            g.setColour(typeColour.withAlpha(state.bypass ? 0.25f : 0.85f));
            g.fillRoundedRectangle(area.withWidth(3.0f).reduced(0.0f, 1.0f), 2.0f);
            g.setColour(state.bypass ? theme.inkSoft.withAlpha(0.5f) : theme.ink);
            g.setFont(uiFont(8.0f));
            g.drawFittedText(juce::String(insertTypeName(state.type)),
                             area.withTrimmedLeft(18.0f).withTrimmedRight(16.0f).toNearestInt(),
                             juce::Justification::centredLeft, 1, 0.7f);
            g.setColour(state.bypass ? theme.inkSoft.withAlpha(0.5f) : typeColour);
            g.fillEllipse(area.getX() + 6.0f, area.getCentreY() - 3.0f, 6.0f, 6.0f);
            g.setColour(theme.inkSoft);
            g.drawText("x", area.getRight() - 14.0f, area.getY(), 12.0f, area.getHeight(),
                       juce::Justification::centred, false);
        }
    }

    g.setColour(theme.trough);
    g.fillRoundedRectangle(panBarBounds, 3.0f);
    const float centre = panBarBounds.getCentreX();
    const float panX = centre + params.pan * panBarBounds.getWidth() * 0.46f;
    g.setColour(colour);
    g.fillRect(juce::Rectangle<float>(juce::jmin(centre, panX), panBarBounds.getY() + 2.0f,
                                      std::abs(panX - centre), panBarBounds.getHeight() - 4.0f));
    g.setColour(theme.inkSoft);
    g.drawVerticalLine(static_cast<int>(centre), panBarBounds.getY(), panBarBounds.getBottom());

    g.setColour(theme.trough);
    g.fillRoundedRectangle(faderTrackBounds, 5.0f);
    drawSketchRect(g, faderTrackBounds, theme.panelEdge, 1.0f, 302);
    const float norm = gainToNormalized(params.level);
    const float thumbY = faderTrackBounds.getBottom() - norm * faderTrackBounds.getHeight();
    faderThumbBounds.setY(thumbY - faderThumbBounds.getHeight() * 0.5f);
    g.setColour(colour);
    g.fillRoundedRectangle(faderThumbBounds, 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(faderThumbBounds, 3.0f, 1.0f);

    g.setColour(theme.trough);
    g.fillRoundedRectangle(meterBounds, 3.0f);
    const float peakL = dbToNormalized(linearToDb(sampler.getFactoryBusPeakL(busIndex_)));
    const float peakR = dbToNormalized(linearToDb(sampler.getFactoryBusPeakR(busIndex_)));
    const float rmsL = dbToNormalized(linearToDb(sampler.getFactoryBusRmsL(busIndex_)));
    const float rmsR = dbToNormalized(linearToDb(sampler.getFactoryBusRmsR(busIndex_)));
    auto meterArea = meterBounds.reduced(3.0f);
    const float halfW = (meterArea.getWidth() - 2.0f) * 0.5f;
    for (int side = 0; side < 2; ++side)
    {
        auto col = meterArea.withWidth(halfW).withX(meterArea.getX() + side * (halfW + 2.0f));
        const float rms = side == 0 ? rmsL : rmsR;
        const float peak = side == 0 ? peakL : peakR;
        const float h = rms * col.getHeight();
        g.setColour(rms > 0.85f ? juce::Colour(0xffe74c3c) : colour);
        g.fillRoundedRectangle(col.withY(col.getBottom() - h).withHeight(h), 2.0f);
        if (peak > 0.02f)
        {
            g.setColour(peak > 0.97f ? juce::Colour(0xffe74c3c) : theme.ink);
            g.fillRect(col.getX(), col.getBottom() - peak * col.getHeight() - 1.0f, col.getWidth(), 2.0f);
        }
    }

    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f));
    g.drawText(formatDb(params.level),
               juce::Rectangle<float>(b.getX(), b.getBottom() - 46.0f, b.getWidth(), 16.0f),
               juce::Justification::centred,
               false);

}

void MixerView::BusStripComponent::mouseDown(const juce::MouseEvent &e)
{
    const auto pos = e.position;
    for (int slot = 0; slot < InsertChain::slotCount; ++slot)
    {
        const auto area = insertBounds[static_cast<size_t>(slot)];
        if (!area.contains(pos))
            continue;

        const auto state = processor.getSampler().getInsertState(true, busIndex_, slot);
        if (state.type == InsertType::none)
        {
            owner.showInsertMenu(true, busIndex_, slot, this, area.toNearestInt());
            return;
        }
        if (pos.x <= area.getX() + 15.0f)
        {
            processor.getSampler().setInsertBypass(true, busIndex_, slot, !state.bypass);
            repaint();
            return;
        }
        if (pos.x >= area.getRight() - 16.0f)
        {
            processor.getSampler().setInsertType(true, busIndex_, slot, InsertType::none);
            repaint();
            return;
        }
        owner.openInsertRack(true, busIndex_, slot);
        return;
    }

    if (panBarBounds.contains(pos))
    {
        draggingPan = true;
        mouseDrag(e);
        return;
    }
    if (faderTrackBounds.expanded(10.0f, 4.0f).contains(pos))
    {
        draggingFader = true;
        mouseDrag(e);
    }
}

void MixerView::BusStripComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto params = processor.getSampler().getFactoryBusParams(busIndex_);
    if (draggingFader)
    {
        const float norm = juce::jlimit(0.0f, 1.0f,
                                        (faderTrackBounds.getBottom() - e.position.y) / faderTrackBounds.getHeight());
        params.level = normalizedToGain(norm);
        processor.getSampler().setFactoryBusParams(busIndex_, params);
    }
    else if (draggingPan)
    {
        params.pan = juce::jlimit(-1.0f, 1.0f,
                                  (e.position.x - panBarBounds.getCentreX()) / (panBarBounds.getWidth() * 0.46f));
        processor.getSampler().setFactoryBusParams(busIndex_, params);
    }
    repaint();
}

void MixerView::BusStripComponent::mouseDoubleClick(const juce::MouseEvent &)
{
    auto params = processor.getSampler().getFactoryBusParams(busIndex_);
    params.level = 1.0f;
    params.pan = 0.0f;
    processor.getSampler().setFactoryBusParams(busIndex_, params);
    repaint();
}

MixerView::InsertRackPopup::InsertRackPopup(MixerView &o, PluginProcessor &p)
    : owner(o), processor(p)
{
    closeBtn.onClick = [this] { close(); };
    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2f2f34));
    closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff2f2f0));
    addAndMakeVisible(closeBtn);
    setInterceptsMouseClicks(true, true);
}

void MixerView::InsertRackPopup::open(bool bus, int channelIndex, int slot)
{
    busTarget = bus;
    channel = channelIndex;
    selectedSlot = juce::jlimit(0, InsertChain::slotCount - 1, slot);
    dragging = false;
    dragFrom = -1;
    dragTo = -1;
    rebuildKnobs();
    setVisible(true);
    toFront(true);
    startTimerHz(30);
    repaint();
}

void MixerView::InsertRackPopup::close()
{
    stopTimer();
    setVisible(false);
    knobs.clear();
}

void MixerView::InsertRackPopup::timerCallback()
{
    if (isVisible())
        repaint();
}

void MixerView::InsertRackPopup::setTheme(const Theme &t)
{
    theme = t;
    closeBtn.setColour(juce::TextButton::buttonColourId, theme.panel);
    closeBtn.setColour(juce::TextButton::textColourOffId, theme.ink);
    for (auto &k : knobs)
        k->setTheme(t);
    repaint();
}

juce::String MixerView::InsertRackPopup::slotTypeLabel(int slot) const
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, slot);
    return juce::String(insertTypeName(state.type));
}

void MixerView::InsertRackPopup::rebuildKnobs()
{
    knobs.clear();
    knobParams.clear();
    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);

    if (state.type == InsertType::eq)
    {
        const int band = juce::jlimit(0, eqBandCount - 1, selectedEqBand);
        const bool cut = eqBandIsCut(band);

        auto addEqKnob = [&](int param, bool logarithmic = false)
        {
            const auto &info = insertParamInfo(InsertType::eq, param);
            auto knob = std::make_unique<Knob>(juce::String(info.name), info.minimum, info.maximum,
                                               info.defaultValue, juce::String(info.unit));
            if (logarithmic)
                knob->setLogarithmic(true);
            knob->setValue(state.values[static_cast<size_t>(param)]);
            knob->setTheme(theme);
            knob->onValueChanged = [this, param](float v)
            {
                processor.getSampler().setInsertValue(busTarget, channel, selectedSlot, param, v);
                repaint();
            };
            addAndMakeVisible(*knob);
            knobs.push_back(std::move(knob));
            knobParams.push_back(param);
        };

        addEqKnob(eqParamFreq(band), true);
        if (!cut)
        {
            addEqKnob(eqParamGain(band));
            addEqKnob(eqParamQ(band));
        }
        addEqKnob(eqParamOutputGain);

        resized();
        repaint();
        return;
    }

    const int count = insertParamCount(state.type);
    if (count <= 0)
    {
        resized();
        repaint();
        return;
    }

    for (int i = 0; i < count; ++i)
    {
        const auto &info = insertParamInfo(state.type, i);
        auto knob = std::make_unique<Knob>(juce::String(info.name), info.minimum, info.maximum, info.defaultValue,
                                           juce::String(info.unit));
        knob->setValue(state.values[static_cast<size_t>(i)]);
        knob->setTheme(theme);
        knob->onValueChanged = [this, i](float v)
        {
            processor.getSampler().setInsertValue(busTarget, channel, selectedSlot, i, v);
            repaint();
        };
        addAndMakeVisible(*knob);
        knobs.push_back(std::move(knob));
        knobParams.push_back(i);
    }

    resized();
    repaint();
}

void MixerView::InsertRackPopup::selectEqBand(int band)
{
    band = juce::jlimit(0, eqBandCount - 1, band);
    if (band == selectedEqBand)
        return;
    selectedEqBand = band;
    rebuildKnobs();
    repaint();
}

void MixerView::InsertRackPopup::setEqValue(int param, float value)
{
    processor.getSampler().setInsertValue(busTarget, channel, selectedSlot, param, value);
}

void MixerView::InsertRackPopup::syncEqKnobs()
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    if (state.type != InsertType::eq)
        return;

    for (size_t i = 0; i < knobs.size() && i < knobParams.size(); ++i)
    {
        const int param = knobParams[i];
        if (param >= 0 && param < maxInsertParams)
            knobs[i]->setValue(state.values[static_cast<size_t>(param)]);
    }
}

int MixerView::InsertRackPopup::slotAt(juce::Point<float> pos) const
{
    for (int i = 0; i < InsertChain::slotCount; ++i)
        if (slotBounds[static_cast<size_t>(i)].contains(pos))
            return i;
    return -1;
}

void MixerView::InsertRackPopup::drawSlot(juce::Graphics &g, int slot, juce::Rectangle<float> area)
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, slot);
    const bool selected = slot == selectedSlot;
    const bool target = dragging && slot == dragTo;
    const auto typeColour = insertColour(state.type);
    const auto accent = state.type == InsertType::none ? theme.panelEdge : typeColour;

    g.setColour(theme.panel.withAlpha(state.type == InsertType::none ? 0.55f : 0.95f));
    g.fillRoundedRectangle(area, 6.0f);
    g.setColour(target ? theme.accentAlt : (selected ? theme.accent : theme.panelEdge));
    g.drawRoundedRectangle(area, 6.0f, selected || target ? 2.0f : 1.0f);

    if (state.type != InsertType::none)
    {
        g.setColour(typeColour.withAlpha(0.9f));
        g.fillRoundedRectangle(area.reduced(1.0f).removeFromTop(5.0f), 3.0f);
    }

    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f).boldened());
    g.drawText("SLOT " + juce::String(slot + 1), area.reduced(8.0f, 5.0f).removeFromTop(12.0f),
               juce::Justification::centredLeft, false);

    g.setColour(state.type == InsertType::none ? theme.inkSoft : theme.ink);
    g.setFont(displayFont(14.0f));
    g.drawFittedText(state.bypass ? (juce::String(insertTypeName(state.type)) + " (BYPASS)")
                                  : juce::String(insertTypeName(state.type)),
                     area.reduced(8.0f, 0.0f).withTrimmedTop(20.0f).withTrimmedBottom(26.0f).toNearestInt(),
                     juce::Justification::centred, 1, 0.7f);

    if (state.type != InsertType::none)
    {
        g.setColour(state.bypass ? theme.warning : typeColour);
        g.setFont(uiFont(8.5f).boldened());
        g.drawText(state.bypass ? "BYPASSED" : "BYPASS", bypassBounds[static_cast<size_t>(slot)].toNearestInt(),
                   juce::Justification::centred, false);
        g.setColour(theme.inkSoft);
        g.drawText("X", removeBounds[static_cast<size_t>(slot)].toNearestInt(), juce::Justification::centred, false);
    }
    else
    {
        g.setColour(theme.isDark ? theme.inkSoft : theme.ink.withAlpha(0.62f));
        g.setFont(uiFont(9.0f).boldened());
        g.drawText("+ ADD INSERT", area.reduced(8.0f).removeFromBottom(20.0f).toNearestInt(),
                   juce::Justification::centred, false);
    }
}

void MixerView::InsertRackPopup::drawEqGraph(juce::Graphics &g, juce::Rectangle<float> area)
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    if (state.type != InsertType::eq)
        return;

    const auto settings = insertEqSettingsFromValues(state.values);
    const auto sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;

    g.setColour(theme.trough);
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(theme.panelEdge);
    g.drawRoundedRectangle(area, 5.0f, 1.0f);

    auto plot = area.reduced(10.0f, 12.0f);
    g.setColour(theme.panelEdge.withAlpha(0.55f));
    for (int i = 0; i <= 4; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * static_cast<float>(i) / 4.0f;
        g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
    }
    for (int i = 1; i < 8; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float>(i) / 8.0f;
        g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
    }

    const auto freqToX = [&](float f)
    {
        const float t = std::log(std::max(20.0f, f) / 20.0f) / std::log(1000.0f);
        return plot.getX() + juce::jlimit(0.0f, 1.0f, t) * plot.getWidth();
    };
    const auto gainToY = [&](float db)
    {
        return plot.getCentreY() - juce::jlimit(-18.0f, 18.0f, db) / 18.0f * plot.getHeight() * 0.5f;
    };

    juce::Path curve;
    for (int i = 0; i <= 220; ++i)
    {
        const float f = 20.0f * std::pow(1000.0f, static_cast<float>(i) / 220.0f);
        const float db = eqTotalMagnitudeDb(settings, f, sampleRate);
        const float x = freqToX(f);
        const float y = gainToY(db);
        if (i == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }

    g.setColour(theme.accent.withAlpha(0.15f));
    auto fill = curve;
    fill.lineTo(plot.getRight(), plot.getBottom());
    fill.lineTo(plot.getX(), plot.getBottom());
    fill.closeSubPath();
    g.fillPath(fill);
    g.setColour(theme.accent);
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    // Every band gets a node; cuts sit on the top or bottom edge.
    for (int band = 0; band < eqBandCount; ++band)
    {
        const auto &b = settings.bands[static_cast<size_t>(band)];
        const bool cut = eqBandIsCut(band);
        const float x = freqToX(b.freq);
        const float y = cut ? (eqBandShape(band) == EqBandShape::lowCut ? plot.getBottom() - 7.0f
                                                                        : plot.getY() + 7.0f)
                            : gainToY(b.gainDb);

        const bool selected = band == selectedEqBand;
        const auto colour = eqBandColour(band);
        const float alpha = b.on ? 1.0f : 0.35f;
        const float radius = selected ? 5.5f : 4.0f;

        g.setColour(colour.withAlpha(alpha * (selected ? 0.9f : 0.65f)));
        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

        if (selected)
        {
            g.setColour(theme.ink.withAlpha(0.9f));
            g.drawEllipse(x - radius - 2.5f, y - radius - 2.5f, radius * 2.0f + 5.0f, radius * 2.0f + 5.0f, 1.2f);
        }

        g.setColour(colour.withAlpha(alpha * 0.9f));
        g.setFont(uiFont(7.0f).boldened());
        g.drawText(eqBandShortName(band), juce::Rectangle<float>(x - 10.0f, y - 16.0f, 20.0f, 10.0f),
                   juce::Justification::centred, false);
    }

    // Numeric readout for the band that is being edited.
    const auto &selectedBand = settings.bands[static_cast<size_t>(selectedEqBand)];
    juce::String readout(eqBandLongName(selectedEqBand));
    readout << "   " << juce::String(selectedBand.freq, selectedBand.freq >= 1000.0f ? 0 : 1) << " Hz";
    if (eqBandHasGain(selectedEqBand))
        readout << "   " << juce::String(selectedBand.gainDb, 1) << " dB   Q "
                << juce::String(selectedBand.q, 2);
    else
        readout << "   " << selectedBand.slope << " dB/oct";
    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f));
    g.drawText(readout, plot.getX() + 4.0f, plot.getY() + 3.0f, plot.getWidth() - 8.0f, 14.0f,
               juce::Justification::left, false);

    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f));
    g.drawText("20", plot.getX(), plot.getBottom() + 2.0f, 30.0f, 12.0f, juce::Justification::left, false);
    g.drawText("1k", plot.getCentreX() - 15.0f, plot.getBottom() + 2.0f, 30.0f, 12.0f,
               juce::Justification::centred, false);
    g.drawText("20k", plot.getRight() - 30.0f, plot.getBottom() + 2.0f, 30.0f, 12.0f,
               juce::Justification::right, false);
}

void MixerView::InsertRackPopup::drawEqControls(juce::Graphics &g)
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    if (state.type != InsertType::eq)
        return;

    for (int band = 0; band < eqBandCount; ++band)
    {
        const auto r = eqChipBounds[static_cast<size_t>(band)];
        if (r.isEmpty())
            continue;

        const bool selected = band == selectedEqBand;
        const bool on = state.values[static_cast<size_t>(eqParamOn(band))] >= 0.5f;
        const auto colour = eqBandColour(band);

        g.setColour(selected ? colour.withAlpha(0.28f) : theme.trough);
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(selected ? colour : theme.panelEdge);
        g.drawRoundedRectangle(r, 3.0f, selected ? 1.6f : 1.0f);

        g.setColour(colour.withAlpha(on ? 1.0f : 0.35f));
        g.setFont(uiFont(9.0f).boldened());
        g.drawText(eqBandShortName(band), r, juce::Justification::centred, false);

        if (!on)
        {
            g.setColour(theme.inkSoft.withAlpha(0.5f));
            g.drawLine(r.getX() + 3.0f, r.getCentreY(), r.getRight() - 3.0f, r.getCentreY(), 1.0f);
        }
    }

    const bool on = state.values[static_cast<size_t>(eqParamOn(selectedEqBand))] >= 0.5f;
    g.setColour(on ? theme.accent.withAlpha(0.25f) : theme.trough);
    g.fillRoundedRectangle(eqOnBounds, 3.0f);
    g.setColour(on ? theme.accent : theme.panelEdge);
    g.drawRoundedRectangle(eqOnBounds, 3.0f, 1.2f);
    g.setColour(on ? theme.ink : theme.inkSoft);
    g.setFont(uiFont(9.0f).boldened());
    g.drawText(on ? "BAND ON" : "BAND OFF", eqOnBounds, juce::Justification::centred, false);

    if (eqBandIsCut(selectedEqBand))
    {
        g.setColour(theme.trough);
        g.fillRoundedRectangle(eqSlopeBounds, 3.0f);
        g.setColour(theme.panelEdge);
        g.drawRoundedRectangle(eqSlopeBounds, 3.0f, 1.0f);
        g.setColour(theme.ink);
        g.setFont(uiFont(9.0f));
        g.drawText("SLOPE  " + juce::String(static_cast<int>(std::lround(
                               state.values[static_cast<size_t>(eqParamSlope(selectedEqBand))])))
                       + " dB/OCT",
                   eqSlopeBounds.reduced(6.0f, 0.0f), juce::Justification::centredLeft, false);
    }
    else
    {
        g.setColour(theme.inkSoft);
        g.setFont(uiFont(9.0f));
        g.drawText(juce::String(eqBandLongName(selectedEqBand)), eqSlopeBounds,
                   juce::Justification::centredLeft, false);
    }
}

void MixerView::InsertRackPopup::drawCompressorMeter(juce::Graphics &g, juce::Rectangle<float> area)
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    g.setColour(theme.trough);
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(theme.panelEdge);
    g.drawRoundedRectangle(area, 5.0f, 1.0f);

    auto inner = area.reduced(18.0f, 16.0f);
    g.setColour(theme.inkSoft);
    g.setFont(uiFont(10.0f).boldened());
    g.drawText("GAIN REDUCTION", inner.removeFromTop(16.0f), juce::Justification::left, false);

    auto meter = inner.removeFromTop(std::min(24.0f, inner.getHeight() * 0.25f)).reduced(0.0f, 3.0f);
    g.setColour(theme.background);
    g.fillRoundedRectangle(meter, 3.0f);
    const float norm = juce::jlimit(0.0f, 1.0f, state.gainReductionDb / 20.0f);
    g.setColour(norm > 0.75f ? juce::Colour(0xffe74c3c) : juce::Colour(0xff52d273));
    g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * norm), 3.0f);

    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f));
    g.drawText(juce::String(state.gainReductionDb, 1) + " dB",
               meter, juce::Justification::centredRight, false);
}

void MixerView::InsertRackPopup::drawClipperCurve(juce::Graphics &g, juce::Rectangle<float> area)
{
    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    g.setColour(theme.trough);
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(theme.panelEdge);
    g.drawRoundedRectangle(area, 5.0f, 1.0f);

    auto plot = area.reduced(14.0f, 14.0f);
    g.setColour(theme.panelEdge.withAlpha(0.6f));
    g.drawRect(plot, 1.0f);
    g.drawHorizontalLine(static_cast<int>(plot.getCentreY()), plot.getX(), plot.getRight());
    g.drawVerticalLine(static_cast<int>(plot.getCentreX()), plot.getY(), plot.getBottom());

    const float drive = 1.0f + state.values[0] * 12.0f;
    const float ceiling = std::pow(10.0f, std::min(0.0f, state.values[1]) / 20.0f);

    juce::Path curve;
    for (int i = 0; i <= 180; ++i)
    {
        const float t = -1.0f + 2.0f * static_cast<float>(i) / 180.0f;
        const float y = std::tanh(t * drive) * ceiling;
        const float px = plot.getCentreX() + t * plot.getWidth() * 0.48f;
        const float py = plot.getCentreY() - y * plot.getHeight() * 0.46f;
        if (i == 0)
            curve.startNewSubPath(px, py);
        else
            curve.lineTo(px, py);
    }
    g.setColour(theme.accent);
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f));
    g.drawText("IN", plot.getX(), plot.getBottom() + 1.0f, 30.0f, 12.0f, juce::Justification::left, false);
    g.drawText("OUT", plot.getRight() - 34.0f, plot.getY() - 13.0f, 34.0f, 12.0f,
               juce::Justification::right, false);
}

int MixerView::InsertRackPopup::eqNodeAt(juce::Point<float> pos, juce::Rectangle<float> area) const
{
    auto plot = area.reduced(10.0f, 12.0f);
    const auto freqToX = [&](float f)
    {
        const float t = std::log(std::max(20.0f, f) / 20.0f) / std::log(1000.0f);
        return plot.getX() + juce::jlimit(0.0f, 1.0f, t) * plot.getWidth();
    };
    const auto gainToY = [&](float db)
    {
        return plot.getCentreY() - juce::jlimit(-18.0f, 18.0f, db) / 18.0f * plot.getHeight() * 0.5f;
    };

    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    int best = -1;
    float bestDistance = 17.0f;

    for (int band = 0; band < eqBandCount; ++band)
    {
        const float freq = state.values[static_cast<size_t>(eqParamFreq(band))];
        const float gain = state.values[static_cast<size_t>(eqParamGain(band))];
        const float x = freqToX(freq);
        const float y = eqBandIsCut(band)
                            ? (eqBandShape(band) == EqBandShape::lowCut ? plot.getBottom() - 7.0f
                                                                        : plot.getY() + 7.0f)
                            : gainToY(gain);
        const float distance = pos.getDistanceFrom({x, y});
        if (distance < bestDistance)
        {
            best = band;
            bestDistance = distance;
        }
    }

    return best;
}

void MixerView::InsertRackPopup::moveEqNode(int band, juce::Point<float> pos, juce::Rectangle<float> area)
{
    if (band < 0 || band >= eqBandCount)
        return;

    auto plot = area.reduced(10.0f, 12.0f);
    const float xNorm = juce::jlimit(0.0f, 1.0f, (pos.x - plot.getX()) / plot.getWidth());
    float freq = 20.0f * std::pow(1000.0f, xNorm);

    if (eqBandIsCut(band))
        freq = band == 0 ? juce::jlimit(20.0f, 1000.0f, freq) : juce::jlimit(1000.0f, 20000.0f, freq);
    else
        freq = juce::jlimit(20.0f, 20000.0f, freq);

    setEqValue(eqParamFreq(band), freq);

    if (eqBandHasGain(band))
    {
        const float db = juce::jlimit(-18.0f, 18.0f,
                                      (plot.getCentreY() - pos.y) / (plot.getHeight() * 0.5f) * 18.0f);
        setEqValue(eqParamGain(band), db);
    }

    syncEqKnobs();
    repaint();
}

void MixerView::InsertRackPopup::paint(juce::Graphics &g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(theme.background.withAlpha(0.97f));
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(theme.panelEdge);
    g.drawRoundedRectangle(b, 10.0f, 1.2f);

    g.setColour(theme.ink);
    g.setFont(displayFont(16.0f));
    const auto title = busTarget ? (channel == 5 ? juce::String("Mix Bus")
                                                 : processor.getSampler().factoryBusTitle(channel))
                                 : processor.getSampler().micStripTitle(channel);
    g.drawText("INSERTS  |  " + title.toUpperCase(), 18, 10, getWidth() - 120, 26,
               juce::Justification::left, false);

    for (int i = 0; i < InsertChain::slotCount; ++i)
        drawSlot(g, i, slotBounds[static_cast<size_t>(i)]);

    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    if (state.type == InsertType::eq)
    {
        drawEqGraph(g, graphBounds);
        drawEqControls(g);
    }
    else if (state.type == InsertType::compressor)
        drawCompressorMeter(g, graphBounds);
    else if (state.type == InsertType::clipper)
        drawClipperCurve(g, graphBounds);

    if (state.type == InsertType::none)
    {
        g.setColour(theme.inkSoft);
        g.setFont(uiFont(12.0f));
        g.drawText("Choose an insert for the selected slot.", 20, 160, getWidth() - 40, 24,
                   juce::Justification::centred, false);
    }

    if (dragging && dragFrom >= 0 && dragTo >= 0 && dragTo != dragFrom)
    {
        g.setColour(theme.accentAlt.withAlpha(0.22f));
        g.fillRoundedRectangle(slotBounds[static_cast<size_t>(dragTo)], 6.0f);
        g.setColour(theme.accentAlt);
        g.drawRoundedRectangle(slotBounds[static_cast<size_t>(dragTo)], 6.0f, 2.0f);
    }
}

void MixerView::InsertRackPopup::resized()
{
    auto b = getLocalBounds().toFloat();
    closeBtn.setBounds(static_cast<int>(b.getRight() - 84.0f), 10, 68, 24);

    const float gap = 8.0f;
    const float slotW = (b.getWidth() - 32.0f - gap * 3.0f) / 4.0f;
    for (int i = 0; i < InsertChain::slotCount; ++i)
    {
        auto area = juce::Rectangle<float>(16.0f + i * (slotW + gap), 42.0f, slotW, 92.0f);
        slotBounds[static_cast<size_t>(i)] = area;
        bypassBounds[static_cast<size_t>(i)] = area.withTrimmedBottom(8.0f).removeFromBottom(20.0f).reduced(8.0f, 0.0f);
        removeBounds[static_cast<size_t>(i)] = {area.getRight() - 24.0f, area.getY() + 4.0f, 18.0f, 18.0f};
    }

    const auto state = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);
    const float margin = 16.0f;
    const float graphW = juce::jmin(400.0f, b.getWidth() * 0.47f);
    graphBounds = {margin, 150.0f, graphW, 190.0f};

    const float areaX = graphBounds.getRight() + 14.0f;
    const float areaW = juce::jmax(120.0f, b.getWidth() - areaX - 14.0f);

    if (state.type == InsertType::eq)
    {
        const float chipW = juce::jmin(30.0f, (areaW - 78.0f) / static_cast<float>(eqBandCount));
        for (int band = 0; band < eqBandCount; ++band)
            eqChipBounds[static_cast<size_t>(band)] = {areaX + static_cast<float>(band) * chipW, 150.0f,
                                                       juce::jmax(14.0f, chipW - 3.0f), 20.0f};

        eqOnBounds = {areaX + static_cast<float>(eqBandCount) * chipW + 6.0f, 150.0f, 66.0f, 20.0f};
        eqSlopeBounds = {areaX, 176.0f, 136.0f, 18.0f};

        if (!knobs.empty())
        {
            const int count = static_cast<int>(knobs.size());
            const float kw = 88.0f;
            const float totalW = static_cast<float>(count) * kw;
            const float start = areaX + juce::jmax(0.0f, (areaW - totalW) * 0.5f);
            for (int i = 0; i < count; ++i)
                knobs[static_cast<size_t>(i)]->setBounds(static_cast<int>(start + static_cast<float>(i) * kw),
                                                         204, 88, 88);
        }
    }
    else if (!knobs.empty())
    {
        const bool hasGraph = state.type == InsertType::compressor || state.type == InsertType::clipper;
        const int count = static_cast<int>(knobs.size());

        if (hasGraph)
        {
            const int cols = juce::jmin(3, count);
            const int rows = (count + cols - 1) / cols;
            const float cellW = areaW / static_cast<float>(cols);
            const float cellH = juce::jmin(92.0f, 190.0f / static_cast<float>(rows));
            const float startY = 150.0f + (190.0f - cellH * static_cast<float>(rows)) * 0.5f;

            for (int i = 0; i < count; ++i)
            {
                const int col = i % cols;
                const int row = i / cols;
                const float kw = juce::jmin(88.0f, cellW - 8.0f);
                const float kh = juce::jmin(84.0f, cellH - 4.0f);
                const float kx = areaX + static_cast<float>(col) * cellW + (cellW - kw) * 0.5f;
                const float ky = startY + static_cast<float>(row) * cellH;
                knobs[static_cast<size_t>(i)]->setBounds(static_cast<int>(kx), static_cast<int>(ky),
                                                         static_cast<int>(kw), static_cast<int>(kh));
            }
        }
        else
        {
            const float cellW = 108.0f;
            const float totalW = count * cellW;
            const float start = juce::jmax(16.0f, (b.getWidth() - totalW) * 0.5f);
            for (int i = 0; i < count; ++i)
                knobs[static_cast<size_t>(i)]->setBounds(static_cast<int>(start + i * cellW), 180, 92, 88);
        }
    }
}

void MixerView::InsertRackPopup::mouseDown(const juce::MouseEvent &e)
{
    const auto selected = processor.getSampler().getInsertState(busTarget, channel, selectedSlot);

    if (selected.type == InsertType::eq)
    {
        for (int band = 0; band < eqBandCount; ++band)
        {
            if (eqChipBounds[static_cast<size_t>(band)].contains(e.position))
            {
                selectEqBand(band);
                return;
            }
        }

        if (eqOnBounds.contains(e.position))
        {
            const int param = eqParamOn(selectedEqBand);
            const float currentOn = selected.values[static_cast<size_t>(param)] >= 0.5f ? 0.0f : 1.0f;
            setEqValue(param, currentOn);
            repaint();
            return;
        }

        if (eqBandIsCut(selectedEqBand) && eqSlopeBounds.contains(e.position))
        {
            static constexpr int slopes[] {6, 12, 18, 24, 30};
            const int param = eqParamSlope(selectedEqBand);
            const int current = static_cast<int>(std::lround(selected.values[static_cast<size_t>(param)]));
            int next = slopes[0];
            for (size_t i = 0; i < std::size(slopes); ++i)
            {
                if (slopes[i] == current)
                {
                    next = slopes[(i + 1) % std::size(slopes)];
                    break;
                }
            }
            setEqValue(param, static_cast<float>(next));
            syncEqKnobs();
            repaint();
            return;
        }

        if (graphBounds.contains(e.position))
        {
            const int node = eqNodeAt(e.position, graphBounds);
            if (node >= 0)
            {
                if (node != selectedEqBand)
                    selectEqBand(node);
                draggingEqNode = node;
                moveEqNode(node, e.position, graphBounds);
                return;
            }
        }
    }

    const int slot = slotAt(e.position);
    if (slot < 0)
        return;

    if (removeBounds[static_cast<size_t>(slot)].contains(e.position))
    {
        processor.getSampler().setInsertType(busTarget, channel, slot, InsertType::none);
        if (selectedSlot == slot)
            rebuildKnobs();
        repaint();
        return;
    }

    if (bypassBounds[static_cast<size_t>(slot)].contains(e.position))
    {
        const auto state = processor.getSampler().getInsertState(busTarget, channel, slot);
        if (state.type != InsertType::none)
        {
            processor.getSampler().setInsertBypass(busTarget, channel, slot, !state.bypass);
            repaint();
        }
        return;
    }

    selectedSlot = slot;
    dragFrom = slot;
    dragStart = e.position;
    dragging = false;
    rebuildKnobs();

    const auto state = processor.getSampler().getInsertState(busTarget, channel, slot);
    if (state.type == InsertType::none)
        owner.showInsertMenu(busTarget, channel, slot, this, slotBounds[static_cast<size_t>(slot)].toNearestInt());
}

void MixerView::InsertRackPopup::mouseDrag(const juce::MouseEvent &e)
{
    if (draggingEqNode >= 0)
    {
        moveEqNode(draggingEqNode, e.position, graphBounds);
        return;
    }

    if (dragFrom < 0)
        return;
    if (!dragging && e.position.getDistanceFrom(dragStart) < 7.0f)
        return;

    dragging = true;
    dragTo = slotAt(e.position);
    repaint();
}

void MixerView::InsertRackPopup::mouseUp(const juce::MouseEvent &)
{
    if (draggingEqNode >= 0)
    {
        draggingEqNode = -1;
        return;
    }

    if (dragging && dragFrom >= 0 && dragTo >= 0 && dragTo != dragFrom)
    {
        processor.getSampler().moveInsert(busTarget, channel, dragFrom, dragTo);
        selectedSlot = dragTo;
        rebuildKnobs();
    }
    dragging = false;
    dragFrom = -1;
    dragTo = -1;
    repaint();
}

MixerView::MasterStripComponent::MasterStripComponent(MixerView &o, PluginProcessor &proc) : owner(o), processor(proc)
{
    limiterBtn.setButtonText("LIMITER");
    limiterBtn.activeColour = juce::Colour(0xff2ecc71);
    limiterBtn.onClick = [this]()
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.limiterEnabled = !params.limiterEnabled;
        processor.getSampler().setMasterStripParams(params);
        limiterBtn.setActive(params.limiterEnabled);
    };
    addAndMakeVisible(limiterBtn);

    fxBtn.setButtonText("MASTER FX");
    fxBtn.activeColour = juce::Colour(0xff9b59b6);
    fxBtn.onClick = [this]()
    {
        owner.closePopups();
        owner.masterFXPopup->open();
    };
    addAndMakeVisible(fxBtn);

    insBtn.setButtonText("INS");
    insBtn.activeColour = juce::Colour(0xff4cc2a8);
    insBtn.onClick = [this]
    {
        owner.closePopups();
        if (owner.insertRackPopup != nullptr)
            owner.insertRackPopup->open(true, 5);
    };
    addAndMakeVisible(insBtn);

    updateFromState();
    setTheme(owner.theme);
}

void MixerView::MasterStripComponent::updateFromState()
{
    const auto params = processor.getSampler().getMasterStripParams();
    limiterBtn.setActive(params.limiterEnabled);
    repaint();
}

void MixerView::MasterStripComponent::resized()
{
    const auto r = getLocalBounds().toFloat();
    const bool isCompact = (r.getHeight() < 300.0f);

    const float btnH = isCompact ? 19.0f : 22.0f;
    limiterBtn.setBounds(r.getX() + 8.0f, isCompact ? 30.0f : 40.0f, r.getWidth() - 16.0f, btnH);
    panBarBounds = juce::Rectangle<float>(
        r.getX() + 8.0f, isCompact ? 52.0f : 68.0f, r.getWidth() - 16.0f, isCompact ? 15.0f : 18.0f);
    const float insW = 42.0f;
    insBtn.setBounds(r.getX() + 8.0f, isCompact ? 70.0f : 92.0f, insW, btnH);
    fxBtn.setBounds(r.getX() + 8.0f + insW + 4.0f, isCompact ? 70.0f : 92.0f,
                    r.getWidth() - 16.0f - insW - 4.0f, btnH);

    const float faderTop = isCompact ? 94.0f : 122.0f;
    const float faderBottom = isCompact ? (r.getBottom() - 28.0f) : (r.getBottom() - 32.0f);
    const float faderH = std::max(30.0f, faderBottom - faderTop);

    faderTrackBounds = juce::Rectangle<float>(r.getX() + 14.0f, faderTop, 28.0f, faderH);
    meterLBounds = juce::Rectangle<float>(r.getX() + 48.0f, faderTop, 12.0f, faderH);
    meterRBounds = juce::Rectangle<float>(r.getX() + 63.0f, faderTop, 12.0f, faderH);
}

void MixerView::MasterStripComponent::setTheme(const Theme &t)
{
    theme = t;
    fxBtn.setTheme(t);
    insBtn.setTheme(t);
    limiterBtn.setTheme(t);
    repaint();
}

void MixerView::MasterStripComponent::paint(juce::Graphics &g)
{
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    const auto params = processor.getSampler().getMasterStripParams();
    const auto ink = theme.ink;

    // Drop shadow (light mode only)
    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.25f));
        g.fillRoundedRectangle(r.translated(3.0f, 3.0f), 6.0f);
    }

    // Master Paper Panel
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 6.0f);
    drawSketchRect(g, r, theme.panelEdge, 2.0f, 90);

    // Master Header Badge: Gold/Amber warm comic pill
    auto badgeR = juce::Rectangle<float>(r.getX() + 4.0f, r.getY() + 4.0f, r.getWidth() - 8.0f, 28.0f);
    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.25f));
        g.fillRoundedRectangle(badgeR.translated(1.5f, 1.5f), 4.0f);
    }
    g.setColour(theme.accent);
    g.fillRoundedRectangle(badgeR, 4.0f);
    drawSketchRect(g, badgeR, theme.isDark ? theme.panelEdge : ink, 1.5f, 91);

    g.setColour(theme.background);
    g.setFont(displayFont(12.5f));
    g.drawText("MIX BUS", badgeR, juce::Justification::centred, false);

    // Master Pan Bar
    g.setColour(theme.trough);
    g.fillRoundedRectangle(panBarBounds, 3.0f);
    drawSketchRect(g, panBarBounds, theme.panelEdge, 1.2f, 92);

    const float centerX = panBarBounds.getCentreX();
    const float panOffset = params.pan * (panBarBounds.getWidth() * 0.45f);
    if (std::abs(params.pan) > 0.02f)
    {
        g.setColour(theme.accent);
        if (params.pan > 0.0f)
            g.fillRect(centerX, panBarBounds.getY() + 2.0f, panOffset, panBarBounds.getHeight() - 4.0f);
        else
            g.fillRect(centerX + panOffset, panBarBounds.getY() + 2.0f, -panOffset, panBarBounds.getHeight() - 4.0f);
    }
    g.setColour(theme.ink);
    g.drawVerticalLine(static_cast<int>(centerX), panBarBounds.getY(), panBarBounds.getBottom());

    // Fader Track: paper trough
    g.setColour(theme.trough);
    g.fillRoundedRectangle(faderTrackBounds, 4.0f);
    drawSketchRect(g, faderTrackBounds, theme.panelEdge, 1.2f, 93);

    // 0dB mark
    const float zeroY = faderTrackBounds.getBottom() - 0.75f * faderTrackBounds.getHeight();
    g.setColour(theme.ink.withAlpha(0.6f));
    g.drawHorizontalLine(static_cast<int>(zeroY), faderTrackBounds.getX() - 3.0f, faderTrackBounds.getX());

    // Dual meters (L and R) with discrete comic segments
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto mR = (ch == 0 ? meterLBounds : meterRBounds);
        g.setColour(theme.trough);
        g.fillRoundedRectangle(mR, 2.0f);
        drawSketchRect(g, mR, theme.panelEdge, 1.0f, static_cast<uint32_t>(94 + ch));

        const float peak =
            (ch == 0 ? processor.getSampler().getMasterPeakL() : processor.getSampler().getMasterPeakR());
        const float rms = (ch == 0 ? processor.getSampler().getMasterRmsL() : processor.getSampler().getMasterRmsR());
        const float normPeak = dbToNormalized(linearToDb(peak));
        const float normRms = dbToNormalized(linearToDb(rms));

        const float mH = mR.getHeight() - 4.0f;
        const float mW = mR.getWidth() - 4.0f;
        const float mX = mR.getX() + 2.0f;
        const float mY = mR.getY() + 2.0f;

        if (normRms > 0.01f)
        {
            constexpr int numSegs = 16;
            const int activeSegs = std::clamp(static_cast<int>(normRms * numSegs), 1, numSegs);
            const float segH = mH / numSegs;

            for (int s = 0; s < activeSegs; ++s)
            {
                const float sy = mY + mH - (s + 1) * segH;
                juce::Colour segCol = juce::Colour(0xff2ecc71); // Green
                if (s >= 13)
                    segCol = juce::Colour(0xffe74c3c); // Red peak
                else if (s >= 10)
                    segCol = juce::Colour(0xfff1c40f); // Amber / Yellow

                g.setColour(segCol);
                g.fillRoundedRectangle(mX, sy + 0.5f, mW, segH - 1.0f, 1.0f);
            }
        }

        if (normPeak > 0.02f)
        {
            const float peakY = mY + mH - normPeak * mH;
            g.setColour(normPeak >= 0.98f ? juce::Colour(0xffe74c3c) : theme.ink);
            g.fillRect(mX, peakY - 1.0f, mW, 2.0f);
        }
    }

    // Master Fader Cap: clean paper cap with ink border & crimson stripe
    const float normGain = gainToNormalized(params.level);
    const float thumbY = faderTrackBounds.getBottom() - normGain * faderTrackBounds.getHeight() - 10.0f;
    auto thumbR =
        juce::Rectangle<float>(faderTrackBounds.getX() - 4.0f, thumbY, faderTrackBounds.getWidth() + 8.0f, 20.0f);

    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.4f));
        g.fillRoundedRectangle(thumbR.translated(2.0f, 2.0f), 3.0f);
    }

    g.setColour(theme.panel);
    g.fillRoundedRectangle(thumbR, 3.0f);
    drawSketchRect(g, thumbR, theme.panelEdge, 1.8f, 97);

    // Crimson master stripe
    g.setColour(juce::Colour(0xffe74c3c));
    g.fillRect(thumbR.getX() + 2.0f, thumbR.getCentreY() - 1.5f, thumbR.getWidth() - 4.0f, 3.0f);

    // Master dB display
    const bool isCompact = (r.getHeight() < 300.0f);
    auto dbRect = juce::Rectangle<float>(r.getX() + 6.0f,
                                         isCompact ? (r.getBottom() - 24.0f) : (r.getBottom() - 26.0f),
                                         r.getWidth() - 12.0f,
                                         isCompact ? 18.0f : 20.0f);
    g.setColour(theme.trough);
    g.fillRoundedRectangle(dbRect, 3.0f);
    drawSketchRect(g, dbRect, theme.panelEdge, 1.2f, 98);

    g.setColour(params.level > 1.0f ? juce::Colour(0xffd03020) : theme.ink);
    g.setFont(displayFont(isCompact ? 9.0f : 10.0f));
    g.drawText(formatDb(params.level), dbRect, juce::Justification::centred, false);
}

void MixerView::MasterStripComponent::mouseDown(const juce::MouseEvent &e)
{
    if (panBarBounds.contains(e.position))
    {
        draggingPan = true;
        mouseDrag(e);
    }
    else if (e.position.y >= faderTrackBounds.getY() - 10.0f && e.position.y <= faderTrackBounds.getBottom() + 10.0f)
    {
        draggingFader = true;
        mouseDrag(e);
    }
}

void MixerView::MasterStripComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (draggingPan)
    {
        const float relX = (e.position.x - panBarBounds.getX()) / panBarBounds.getWidth();
        const float newPan = std::clamp(relX * 2.0f - 1.0f, -1.0f, 1.0f);
        auto params = processor.getSampler().getMasterStripParams();
        params.pan = newPan;
        processor.getSampler().setMasterStripParams(params);
        repaint();
    }
    else if (draggingFader)
    {
        const float norm =
            std::clamp((faderTrackBounds.getBottom() - e.position.y) / faderTrackBounds.getHeight(), 0.0f, 1.0f);
        const float newLevel = normalizedToGain(norm);
        auto params = processor.getSampler().getMasterStripParams();
        params.level = newLevel;
        processor.getSampler().setMasterStripParams(params);
        repaint();
    }
}

void MixerView::MasterStripComponent::mouseDoubleClick(const juce::MouseEvent &e)
{
    if (panBarBounds.contains(e.position))
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.pan = 0.0f;
        processor.getSampler().setMasterStripParams(params);
        repaint();
    }
    else if (faderTrackBounds.expanded(10.0f, 0.0f).contains(e.position))
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.level = 0.85f;
        processor.getSampler().setMasterStripParams(params);
        repaint();
    }
}

//==============================================================================
// ChannelFXPopup
//==============================================================================

MixerView::ChannelFXPopup::ChannelFXPopup(MixerView &o, PluginProcessor &proc) : owner(o), processor(proc)
{
    closeBtn.onClick = [this]() { close(); };
    addAndMakeVisible(closeBtn);

    eqToggle.onClick = [this]()
    {
        if (currentChannel >= 0)
        {
            auto params = processor.getSampler().getMicStripParams(currentChannel);
            params.eqEnabled = !params.eqEnabled;
            processor.getSampler().setMicStripParams(currentChannel, params);
            eqToggle.setActive(params.eqEnabled);
        }
    };
    addAndMakeVisible(eqToggle);

    compToggle.onClick = [this]()
    {
        if (currentChannel >= 0)
        {
            auto params = processor.getSampler().getMicStripParams(currentChannel);
            params.compEnabled = !params.compEnabled;
            processor.getSampler().setMicStripParams(currentChannel, params);
            compToggle.setActive(params.compEnabled);
        }
    };
    addAndMakeVisible(compToggle);

    // Build Knobs
    eqLowGainKnob = std::make_unique<Knob>("LOW 80Hz", -12.0f, 12.0f, 0.0f, "dB");
    eqMidGainKnob = std::make_unique<Knob>("MID GAIN", -12.0f, 12.0f, 0.0f, "dB");
    eqMidFreqKnob = std::make_unique<Knob>("MID FREQ", 200.0f, 6000.0f, 1000.0f, "Hz");
    eqHighGainKnob = std::make_unique<Knob>("HIGH 10k", -12.0f, 12.0f, 0.0f, "dB");

    compThreshKnob = std::make_unique<Knob>("THRESH", -30.0f, 0.0f, -14.0f, "dB");
    compRatioKnob = std::make_unique<Knob>("RATIO", 1.0f, 8.0f, 3.0f, ":1");
    compAttackKnob = std::make_unique<Knob>("ATTACK", 1.0f, 50.0f, 15.0f, "ms");
    compReleaseKnob = std::make_unique<Knob>("RELEASE", 20.0f, 300.0f, 100.0f, "ms");
    compMakeupKnob = std::make_unique<Knob>("MAKEUP", 0.0f, 12.0f, 0.0f, "dB");

    hpfKnob = std::make_unique<Knob>("HPF", 20.0f, 500.0f, 20.0f, "Hz");
    lpfKnob = std::make_unique<Knob>("LPF", 2000.0f, 20000.0f, 20000.0f, "Hz");

    satDriveKnob = std::make_unique<Knob>("SAT DRIVE", 0.0f, 1.0f, 0.0f, "");
    transAttackKnob = std::make_unique<Knob>("ATTACK", -1.0f, 1.0f, 0.0f, "");
    transSustainKnob = std::make_unique<Knob>("SUSTAIN", -1.0f, 1.0f, 0.0f, "");

    auto bindKnob = [this](Knob &k, auto setter)
    {
        k.onValueChanged = [this, setter](float v)
        {
            if (currentChannel >= 0)
            {
                auto params = processor.getSampler().getMicStripParams(currentChannel);
                setter(params, v);
                processor.getSampler().setMicStripParams(currentChannel, params);
            }
        };
    };

    bindKnob(*eqLowGainKnob, [](auto &p, float v) { p.eqLowGainDb = v; });
    bindKnob(*eqMidGainKnob, [](auto &p, float v) { p.eqMidGainDb = v; });
    bindKnob(*eqMidFreqKnob, [](auto &p, float v) { p.eqMidFreq = v; });
    bindKnob(*eqHighGainKnob, [](auto &p, float v) { p.eqHighGainDb = v; });

    bindKnob(*compThreshKnob, [](auto &p, float v) { p.compThresholdDb = v; });
    bindKnob(*compRatioKnob, [](auto &p, float v) { p.compRatio = v; });
    bindKnob(*compAttackKnob, [](auto &p, float v) { p.compAttackMs = v; });
    bindKnob(*compReleaseKnob, [](auto &p, float v) { p.compReleaseMs = v; });
    bindKnob(*compMakeupKnob, [](auto &p, float v) { p.compMakeupDb = v; });

    bindKnob(*hpfKnob, [](auto &p, float v) { p.hpfHz = v; });
    bindKnob(*lpfKnob, [](auto &p, float v) { p.lpfHz = v; });

    bindKnob(*satDriveKnob, [](auto &p, float v) { p.saturationDrive = v; });
    bindKnob(*transAttackKnob, [](auto &p, float v) { p.transientAttack = v; });
    bindKnob(*transSustainKnob, [](auto &p, float v) { p.transientSustain = v; });

    addAndMakeVisible(*eqLowGainKnob);
    addAndMakeVisible(*eqMidGainKnob);
    addAndMakeVisible(*eqMidFreqKnob);
    addAndMakeVisible(*eqHighGainKnob);

    addAndMakeVisible(*compThreshKnob);
    addAndMakeVisible(*compRatioKnob);
    addAndMakeVisible(*compAttackKnob);
    addAndMakeVisible(*compReleaseKnob);
    addAndMakeVisible(*compMakeupKnob);

    addAndMakeVisible(*hpfKnob);
    addAndMakeVisible(*lpfKnob);

    addAndMakeVisible(*satDriveKnob);
    addAndMakeVisible(*transAttackKnob);
    addAndMakeVisible(*transSustainKnob);

    setVisible(false);
    setTheme(owner.theme);
}

void MixerView::ChannelFXPopup::openForChannel(int chIdx)
{
    currentChannel = chIdx;
    if (currentChannel < 0 || currentChannel >= processor.getSampler().numMicStrips())
        return;

    const auto p = processor.getSampler().getMicStripParams(currentChannel);
    eqToggle.setActive(p.eqEnabled);
    compToggle.setActive(p.compEnabled);

    eqLowGainKnob->setValue(p.eqLowGainDb);
    eqMidGainKnob->setValue(p.eqMidGainDb);
    eqMidFreqKnob->setValue(p.eqMidFreq);
    eqHighGainKnob->setValue(p.eqHighGainDb);

    compThreshKnob->setValue(p.compThresholdDb);
    compRatioKnob->setValue(p.compRatio);
    compAttackKnob->setValue(p.compAttackMs);
    compReleaseKnob->setValue(p.compReleaseMs);
    compMakeupKnob->setValue(p.compMakeupDb);

    hpfKnob->setValue(p.hpfHz);
    lpfKnob->setValue(p.lpfHz);

    satDriveKnob->setValue(p.saturationDrive);
    transAttackKnob->setValue(p.transientAttack);
    transSustainKnob->setValue(p.transientSustain);

    setVisible(true);
    toFront(true);
    resized();
    repaint();
}

void MixerView::ChannelFXPopup::close()
{
    setVisible(false);
    currentChannel = -1;
    owner.refreshChannels();
}

void MixerView::ChannelFXPopup::resized()
{
    closeBtn.setBounds(getWidth() - 90, 14, 76, 24);

    // Section 1: 3-Band Parametric EQ
    eqToggle.setBounds(24, 52, 90, 22);
    eqLowGainKnob->setBounds(124, 46, 60, 60);
    eqMidFreqKnob->setBounds(194, 46, 60, 60);
    eqMidGainKnob->setBounds(264, 46, 60, 60);
    eqHighGainKnob->setBounds(334, 46, 60, 60);

    // Section 2: VCA Compressor
    compToggle.setBounds(24, 126, 90, 22);
    compThreshKnob->setBounds(124, 120, 60, 60);
    compRatioKnob->setBounds(194, 120, 60, 60);
    compAttackKnob->setBounds(264, 120, 60, 60);
    compReleaseKnob->setBounds(334, 120, 60, 60);
    compMakeupKnob->setBounds(404, 120, 60, 60);

    // Section 3: Saturation & Transient Shaper
    satDriveKnob->setBounds(124, 194, 60, 60);
    transAttackKnob->setBounds(244, 194, 60, 60);
    transSustainKnob->setBounds(314, 194, 60, 60);

    // Section 4: Filters
    hpfKnob->setBounds(124, 266, 60, 60);
    lpfKnob->setBounds(204, 266, 60, 60);
}

void MixerView::ChannelFXPopup::setTheme(const Theme &t)
{
    theme = t;
    closeBtn.setTheme(t);
    eqToggle.setTheme(t);
    compToggle.setTheme(t);
    if (eqLowGainKnob)
        eqLowGainKnob->setTheme(t);
    if (eqMidFreqKnob)
        eqMidFreqKnob->setTheme(t);
    if (eqMidGainKnob)
        eqMidGainKnob->setTheme(t);
    if (eqHighGainKnob)
        eqHighGainKnob->setTheme(t);
    if (compThreshKnob)
        compThreshKnob->setTheme(t);
    if (compRatioKnob)
        compRatioKnob->setTheme(t);
    if (compAttackKnob)
        compAttackKnob->setTheme(t);
    if (compReleaseKnob)
        compReleaseKnob->setTheme(t);
    if (compMakeupKnob)
        compMakeupKnob->setTheme(t);
    if (hpfKnob)
        hpfKnob->setTheme(t);
    if (lpfKnob)
        lpfKnob->setTheme(t);
    if (satDriveKnob)
        satDriveKnob->setTheme(t);
    if (transAttackKnob)
        transAttackKnob->setTheme(t);
    if (transSustainKnob)
        transSustainKnob->setTheme(t);
    repaint();
}

void MixerView::ChannelFXPopup::paint(juce::Graphics &g)
{
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    const auto ink = theme.ink;

    // Backdrop shadow (light mode only)
    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.35f));
        g.fillRoundedRectangle(r.translated(4.0f, 4.0f), 8.0f);
    }

    // Card background
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 8.0f);
    drawSketchRect(g, r, theme.panelEdge, 2.2f, 101);

    // Header banner
    auto headerR = juce::Rectangle<float>(r.getX() + 4.0f, r.getY() + 4.0f, r.getWidth() - 8.0f, 32.0f);
    g.setColour(juce::Colour(0xffffeaa7)); // Golden banner
    g.fillRoundedRectangle(headerR, 6.0f);
    drawSketchRect(g, headerR, theme.isDark ? theme.panelEdge : ink, 1.4f, 102);

    const auto name = currentChannel >= 0 ? processor.getSampler().micStripTitle(currentChannel) : "";
    g.setColour(juce::Colour(0xff101114));
    g.setFont(displayFont(16.0f));
    g.drawText(
        "CHANNEL FX RACK: " + name.toUpperCase(), 24, 14, getWidth() - 120, 24, juce::Justification::left, false);

    // Dividing lines
    g.setColour(theme.ink.withAlpha(0.25f));
    g.drawHorizontalLine(112, 20.0f, getWidth() - 20.0f);
    g.drawHorizontalLine(186, 20.0f, getWidth() - 20.0f);
    g.drawHorizontalLine(258, 20.0f, getWidth() - 20.0f);

    // Section labels
    g.setColour(theme.ink);
    g.setFont(displayFont(11.5f));
    g.drawText("3-BAND EQ", 24, 78, 90, 20, juce::Justification::left, false);
    g.drawText("VCA COMP", 24, 152, 90, 20, juce::Justification::left, false);
    g.drawText("TAPE & TRANSIENT", 24, 214, 96, 20, juce::Justification::left, false);
    g.drawText("FILTERS", 24, 286, 90, 20, juce::Justification::left, false);
}

void MixerView::ChannelFXPopup::mouseDown(const juce::MouseEvent &e)
{
    juce::ignoreUnused(e);
}

//==============================================================================
// MasterFXPopup
//==============================================================================

MixerView::MasterFXPopup::MasterFXPopup(MixerView &o, PluginProcessor &proc) : owner(o), processor(proc)
{
    closeBtn.onClick = [this]() { close(); };
    addAndMakeVisible(closeBtn);

    limiterToggle.onClick = [this]()
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.limiterEnabled = !params.limiterEnabled;
        processor.getSampler().setMasterStripParams(params);
        limiterToggle.setActive(params.limiterEnabled);
    };
    addAndMakeVisible(limiterToggle);

    busCompToggle.onClick = [this]()
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.busCompEnabled = !params.busCompEnabled;
        processor.getSampler().setMasterStripParams(params);
        busCompToggle.setActive(params.busCompEnabled);
    };
    addAndMakeVisible(busCompToggle);

    busLowKnob = std::make_unique<Knob>("LOW", -12.0f, 12.0f, 0.0f, "dB");
    busMidKnob = std::make_unique<Knob>("MID", -12.0f, 12.0f, 0.0f, "dB");
    busMidFreqKnob = std::make_unique<Knob>("MID FREQ", 100.0f, 10000.0f, 800.0f, "Hz");
    busHighKnob = std::make_unique<Knob>("HIGH", -12.0f, 12.0f, 0.0f, "dB");
    busThreshKnob = std::make_unique<Knob>("THRESH", -30.0f, 0.0f, -12.0f, "dB");
    busRatioKnob = std::make_unique<Knob>("RATIO", 1.0f, 8.0f, 2.0f, ":1");
    busAttackKnob = std::make_unique<Knob>("ATTACK", 1.0f, 100.0f, 30.0f, "ms");
    busReleaseKnob = std::make_unique<Knob>("RELEASE", 20.0f, 500.0f, 150.0f, "ms");
    busMakeupKnob = std::make_unique<Knob>("MAKEUP", 0.0f, 12.0f, 0.0f, "dB");
    busMixKnob = std::make_unique<Knob>("PARALLEL", 0.0f, 1.0f, 1.0f, "");
    busSatKnob = std::make_unique<Knob>("DRIVE", 0.0f, 1.0f, 0.0f, "");

    auto bindBusKnob = [this](Knob &k, auto setter)
    {
        k.onValueChanged = [this, setter](float v)
        {
            auto params = processor.getSampler().getMasterStripParams();
            setter(params, v);
            processor.getSampler().setMasterStripParams(params);
        };
    };

    bindBusKnob(*busLowKnob, [](auto &p, float v) { p.busEqLowDb = v; });
    bindBusKnob(*busMidKnob, [](auto &p, float v) { p.busEqMidDb = v; });
    bindBusKnob(*busMidFreqKnob, [](auto &p, float v) { p.busEqMidFreq = v; });
    bindBusKnob(*busHighKnob, [](auto &p, float v) { p.busEqHighDb = v; });
    bindBusKnob(*busThreshKnob, [](auto &p, float v) { p.busCompThresholdDb = v; });
    bindBusKnob(*busRatioKnob, [](auto &p, float v) { p.busCompRatio = v; });
    bindBusKnob(*busAttackKnob, [](auto &p, float v) { p.busCompAttackMs = v; });
    bindBusKnob(*busReleaseKnob, [](auto &p, float v) { p.busCompReleaseMs = v; });
    bindBusKnob(*busMakeupKnob, [](auto &p, float v) { p.busCompMakeupDb = v; });
    bindBusKnob(*busMixKnob, [](auto &p, float v) { p.busCompMix = v; });
    bindBusKnob(*busSatKnob, [](auto &p, float v) { p.busSaturationDrive = v; });

    revReturnKnob = std::make_unique<Knob>("WET", 0.0f, 1.0f, 0.25f, "");
    revSizeKnob = std::make_unique<Knob>("ROOM SIZE", 0.0f, 1.0f, 0.45f, "");
    revDampKnob = std::make_unique<Knob>("DAMPING", 0.0f, 1.0f, 0.40f, "");
    revWidthKnob = std::make_unique<Knob>("WIDTH", 0.0f, 1.0f, 0.85f, "");

    revReturnKnob->onValueChanged = [this](float v)
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.reverbReturn = v;
        processor.getSampler().setMasterStripParams(params);
    };
    revSizeKnob->onValueChanged = [this](float v)
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.reverbSize = v;
        processor.getSampler().setMasterStripParams(params);
    };
    revDampKnob->onValueChanged = [this](float v)
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.reverbDamp = v;
        processor.getSampler().setMasterStripParams(params);
    };
    revWidthKnob->onValueChanged = [this](float v)
    {
        auto params = processor.getSampler().getMasterStripParams();
        params.reverbWidth = v;
        processor.getSampler().setMasterStripParams(params);
    };

    addAndMakeVisible(*busLowKnob);
    addAndMakeVisible(*busMidKnob);
    addAndMakeVisible(*busMidFreqKnob);
    addAndMakeVisible(*busHighKnob);
    addAndMakeVisible(*busThreshKnob);
    addAndMakeVisible(*busRatioKnob);
    addAndMakeVisible(*busAttackKnob);
    addAndMakeVisible(*busReleaseKnob);
    addAndMakeVisible(*busMakeupKnob);
    addAndMakeVisible(*busMixKnob);
    addAndMakeVisible(*busSatKnob);

    addAndMakeVisible(*revReturnKnob);
    addAndMakeVisible(*revSizeKnob);
    addAndMakeVisible(*revDampKnob);
    addAndMakeVisible(*revWidthKnob);

    setVisible(false);
    setTheme(owner.theme);
}

void MixerView::MasterFXPopup::open()
{
    const auto p = processor.getSampler().getMasterStripParams();
    limiterToggle.setActive(p.limiterEnabled);
    busCompToggle.setActive(p.busCompEnabled);
    busLowKnob->setValue(p.busEqLowDb);
    busMidKnob->setValue(p.busEqMidDb);
    busMidFreqKnob->setValue(p.busEqMidFreq);
    busHighKnob->setValue(p.busEqHighDb);
    busThreshKnob->setValue(p.busCompThresholdDb);
    busRatioKnob->setValue(p.busCompRatio);
    busAttackKnob->setValue(p.busCompAttackMs);
    busReleaseKnob->setValue(p.busCompReleaseMs);
    busMakeupKnob->setValue(p.busCompMakeupDb);
    busMixKnob->setValue(p.busCompMix);
    busSatKnob->setValue(p.busSaturationDrive);
    revReturnKnob->setValue(p.reverbReturn);
    revSizeKnob->setValue(p.reverbSize);
    revDampKnob->setValue(p.reverbDamp);
    revWidthKnob->setValue(p.reverbWidth);

    setVisible(true);
    toFront(true);
    resized();
    repaint();
}

void MixerView::MasterFXPopup::close()
{
    setVisible(false);
    owner.refreshChannels();
}

void MixerView::MasterFXPopup::resized()
{
    closeBtn.setBounds(getWidth() - 90, 14, 76, 24);
    limiterToggle.setBounds(24, 52, 100, 24);
    busCompToggle.setBounds(134, 52, 110, 24);

    busLowKnob->setBounds(24, 100, 60, 60);
    busMidKnob->setBounds(94, 100, 60, 60);
    busMidFreqKnob->setBounds(164, 100, 60, 60);
    busHighKnob->setBounds(234, 100, 60, 60);

    busThreshKnob->setBounds(24, 186, 60, 60);
    busRatioKnob->setBounds(94, 186, 60, 60);
    busAttackKnob->setBounds(164, 186, 60, 60);
    busReleaseKnob->setBounds(234, 186, 60, 60);
    busMakeupKnob->setBounds(304, 186, 60, 60);
    busMixKnob->setBounds(374, 186, 60, 60);

    busSatKnob->setBounds(24, 272, 60, 60);
    revReturnKnob->setBounds(94, 272, 60, 60);
    revSizeKnob->setBounds(164, 272, 60, 60);
    revDampKnob->setBounds(234, 272, 60, 60);
    revWidthKnob->setBounds(304, 272, 60, 60);
}

void MixerView::MasterFXPopup::setTheme(const Theme &t)
{
    theme = t;
    closeBtn.setTheme(t);
    limiterToggle.setTheme(t);
    busCompToggle.setTheme(t);
    if (busLowKnob)
        busLowKnob->setTheme(t);
    if (busMidKnob)
        busMidKnob->setTheme(t);
    if (busMidFreqKnob)
        busMidFreqKnob->setTheme(t);
    if (busHighKnob)
        busHighKnob->setTheme(t);
    if (busThreshKnob)
        busThreshKnob->setTheme(t);
    if (busRatioKnob)
        busRatioKnob->setTheme(t);
    if (busAttackKnob)
        busAttackKnob->setTheme(t);
    if (busReleaseKnob)
        busReleaseKnob->setTheme(t);
    if (busMakeupKnob)
        busMakeupKnob->setTheme(t);
    if (busMixKnob)
        busMixKnob->setTheme(t);
    if (busSatKnob)
        busSatKnob->setTheme(t);
    if (revReturnKnob)
        revReturnKnob->setTheme(t);
    if (revSizeKnob)
        revSizeKnob->setTheme(t);
    if (revDampKnob)
        revDampKnob->setTheme(t);
    if (revWidthKnob)
        revWidthKnob->setTheme(t);
    repaint();
}

void MixerView::MasterFXPopup::paint(juce::Graphics &g)
{
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    const auto ink = theme.ink;

    // Backdrop shadow (light mode only)
    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.35f));
        g.fillRoundedRectangle(r.translated(4.0f, 4.0f), 8.0f);
    }

    // Card background
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 8.0f);
    drawSketchRect(g, r, theme.panelEdge, 2.2f, 103);

    // Header banner
    auto headerR = juce::Rectangle<float>(r.getX() + 4.0f, r.getY() + 4.0f, r.getWidth() - 8.0f, 32.0f);
    g.setColour(theme.isDark ? juce::Colour(0xff3a3c44) : juce::Colour(0xffdfe6e9));
    g.fillRoundedRectangle(headerR, 6.0f);
    drawSketchRect(g, headerR, theme.isDark ? theme.panelEdge : ink, 1.4f, 104);

    g.setColour(theme.isDark ? juce::Colours::white : ink);
    g.setFont(displayFont(15.0f));
    g.drawText("MIX BUS & REVERB", 24, 14, getWidth() - 120, 24, juce::Justification::left, false);

    g.setColour(theme.ink.withAlpha(0.25f));
    g.drawHorizontalLine(88, 20.0f, getWidth() - 20.0f);
    g.drawHorizontalLine(174, 20.0f, getWidth() - 20.0f);
    g.drawHorizontalLine(260, 20.0f, getWidth() - 20.0f);

    g.setColour(theme.ink);
    g.setFont(displayFont(11.5f));
    g.drawText("BUS EQ", 24, 88, 90, 12, juce::Justification::left, false);
    g.drawText("GLUE COMP", 24, 174, 90, 12, juce::Justification::left, false);
    g.drawText("SAT", 24, 260, 60, 12, juce::Justification::left, false);
    g.drawText("REVERB", 94, 260, 90, 12, juce::Justification::left, false);
}

void MixerView::MasterFXPopup::mouseDown(const juce::MouseEvent &e)
{
    juce::ignoreUnused(e);
}

//==============================================================================
// Vector Drum Piece Icon Renderer (Hand-drawn comic/sketch style)
//==============================================================================
static void drawPieceVectorIcon(
    juce::Graphics &g, BleedPiece piece, juce::Rectangle<float> bounds, juce::Colour colour, float strokeWidth)
{
    g.setColour(colour);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();

    switch (piece)
    {
        case BleedPiece::kick:
        {
            const float r = std::min(bounds.getWidth(), bounds.getHeight()) * 0.38f;
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, strokeWidth);
            g.fillEllipse(cx - r * 0.28f, cy - r * 0.28f, r * 0.56f, r * 0.56f);
            juce::Path spurs;
            spurs.startNewSubPath(cx - r * 0.707f, cy + r * 0.707f);
            spurs.lineTo(cx - r * 1.15f, cy + r * 1.15f);
            spurs.startNewSubPath(cx + r * 0.707f, cy + r * 0.707f);
            spurs.lineTo(cx + r * 1.15f, cy + r * 1.15f);
            g.strokePath(
                spurs, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::snare:
        {
            const float w = bounds.getWidth() * 0.82f;
            const float h = bounds.getHeight() * 0.52f;
            const float x = cx - w * 0.5f;
            const float y = cy - h * 0.5f;
            g.drawEllipse(x, y, w, h * 0.45f, strokeWidth);
            juce::Path shell;
            shell.startNewSubPath(x, y + h * 0.225f);
            shell.lineTo(x, y + h);
            shell.startNewSubPath(x + w, y + h * 0.225f);
            shell.lineTo(x + w, y + h);
            shell.startNewSubPath(x, y + h);
            shell.quadraticTo(cx, y + h + h * 0.25f, x + w, y + h);
            // Snare wires across bottom
            shell.startNewSubPath(x + w * 0.18f, y + h * 0.65f);
            shell.lineTo(x + w * 0.34f, y + h * 0.52f);
            shell.lineTo(x + w * 0.50f, y + h * 0.65f);
            shell.lineTo(x + w * 0.66f, y + h * 0.52f);
            shell.lineTo(x + w * 0.82f, y + h * 0.65f);
            g.strokePath(
                shell, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::tom1:
        case BleedPiece::tom2:
        {
            const float scale = (piece == BleedPiece::tom1 ? 0.72f : 0.80f);
            const float w = bounds.getWidth() * scale;
            const float h = bounds.getHeight() * (scale * 0.75f);
            const float x = cx - w * 0.5f;
            const float y = cy - h * 0.5f;
            g.drawEllipse(x, y, w, h * 0.40f, strokeWidth);
            juce::Path shell;
            shell.startNewSubPath(x, y + h * 0.20f);
            shell.lineTo(x, y + h);
            shell.startNewSubPath(x + w, y + h * 0.20f);
            shell.lineTo(x + w, y + h);
            shell.startNewSubPath(x, y + h);
            shell.quadraticTo(cx, y + h + h * 0.25f, x + w, y + h);
            g.strokePath(
                shell, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::floorTom:
        {
            const float w = bounds.getWidth() * 0.80f;
            const float h = bounds.getHeight() * 0.58f;
            const float x = cx - w * 0.5f;
            const float y = cy - h * 0.55f;
            g.drawEllipse(x, y, w, h * 0.38f, strokeWidth);
            juce::Path shell;
            shell.startNewSubPath(x, y + h * 0.19f);
            shell.lineTo(x, y + h);
            shell.startNewSubPath(x + w, y + h * 0.19f);
            shell.lineTo(x + w, y + h);
            shell.startNewSubPath(x, y + h);
            shell.quadraticTo(cx, y + h + h * 0.22f, x + w, y + h);
            // 3 Floor Tom legs
            shell.startNewSubPath(x + w * 0.15f, y + h * 0.6f);
            shell.lineTo(x - 2.0f, bounds.getBottom() - 1.0f);
            shell.startNewSubPath(x + w * 0.85f, y + h * 0.6f);
            shell.lineTo(x + w + 2.0f, bounds.getBottom() - 1.0f);
            shell.startNewSubPath(cx, y + h * 0.8f);
            shell.lineTo(cx, bounds.getBottom() - 1.0f);
            g.strokePath(
                shell, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::hihat:
        {
            const float w = bounds.getWidth() * 0.82f;
            g.drawLine(cx, bounds.getY() + 1.0f, cx, bounds.getBottom() - 1.0f, strokeWidth);
            juce::Path cymbals;
            // Top cymbal arc
            cymbals.startNewSubPath(cx - w * 0.5f, cy - 2.5f);
            cymbals.quadraticTo(cx, cy - 6.0f, cx + w * 0.5f, cy - 2.5f);
            // Bottom cymbal arc
            cymbals.startNewSubPath(cx - w * 0.5f, cy + 2.5f);
            cymbals.quadraticTo(cx, cy + 6.0f, cx + w * 0.5f, cy + 2.5f);
            // Stand pedal base line
            cymbals.startNewSubPath(cx - w * 0.35f, bounds.getBottom() - 1.0f);
            cymbals.lineTo(cx + w * 0.35f, bounds.getBottom() - 1.0f);
            g.strokePath(
                cymbals,
                juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::ride:
        {
            const float w = bounds.getWidth() * 0.86f;
            juce::Path p;
            p.startNewSubPath(cx - w * 0.5f, cy + 2.0f);
            p.quadraticTo(cx - w * 0.14f, cy - 3.5f, cx - w * 0.10f, cy - 3.5f);
            p.quadraticTo(cx, cy - 8.0f, cx + w * 0.10f, cy - 3.5f);
            p.quadraticTo(cx + w * 0.14f, cy - 3.5f, cx + w * 0.5f, cy + 2.0f);
            p.startNewSubPath(cx, cy - 1.0f);
            p.lineTo(cx, bounds.getBottom() - 1.0f);
            g.strokePath(
                p, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::crash1:
        case BleedPiece::crash2:
        {
            const float w = bounds.getWidth() * 0.82f;
            const float tilt = (piece == BleedPiece::crash1 ? -2.5f : 2.5f);
            juce::Path p;
            p.startNewSubPath(cx - w * 0.5f, cy + tilt + 1.5f);
            p.quadraticTo(cx, cy + tilt - 4.5f, cx + w * 0.5f, cy - tilt - 1.5f);
            p.startNewSubPath(cx, cy);
            p.lineTo(cx, bounds.getBottom() - 1.0f);
            // Vibrato slash accents
            p.startNewSubPath(cx - w * 0.35f, cy - 5.0f);
            p.lineTo(cx - w * 0.45f, cy - 8.0f);
            p.startNewSubPath(cx + w * 0.35f, cy - 5.0f);
            p.lineTo(cx + w * 0.45f, cy - 8.0f);
            g.strokePath(
                p, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case BleedPiece::count:
        default:
            break;
    }
}

//==============================================================================
// BleedMatrixPopup
//==============================================================================

MixerView::PresetBrowserPopup::PresetBrowserPopup(MixerView &o, PluginProcessor &proc) : owner(o), processor(proc)
{
    closeBtn.onClick = [this]() { close(); };
    addAndMakeVisible(closeBtn);

    favouriteBtn.onClick = [this]()
    {
        if (selectedEntry < 0 || selectedEntry >= static_cast<int>(entries.size()))
            return;

        entries[static_cast<size_t>(selectedEntry)].favourite =
            !entries[static_cast<size_t>(selectedEntry)].favourite;
        saveFavourites();
        rebuildRows();
        repaint();
    };
    addAndMakeVisible(favouriteBtn);

    slotABtn.onClick = [this]() { recallSlot(0); };
    slotBBtn.onClick = [this]() { recallSlot(1); };
    storeABtn.onClick = [this]()
    {
        activeSlot = 0;
        storeCurrentInActiveSlot();
    };
    storeBBtn.onClick = [this]()
    {
        activeSlot = 1;
        storeCurrentInActiveSlot();
    };
    addAndMakeVisible(slotABtn);
    addAndMakeVisible(slotBBtn);
    addAndMakeVisible(storeABtn);
    addAndMakeVisible(storeBBtn);

    listViewport.setViewedComponent(&listContent, false);
    listViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(listViewport);

    setVisible(false);
    setTheme(owner.theme);
}

juce::File MixerView::PresetBrowserPopup::favouritesFile() const
{
    return processor.getSettings().getConfigDirectory().getChildFile("mix_presets").getChildFile("favourites.txt");
}

void MixerView::PresetBrowserPopup::loadFavourites()
{
    const auto file = favouritesFile();
    if (!file.existsAsFile())
        return;

    juce::StringArray lines;
    lines.addLines(file.loadFileAsString());

    for (auto &entry : entries)
        entry.favourite = lines.contains(entry.name, true);
}

void MixerView::PresetBrowserPopup::saveFavourites() const
{
    auto file = favouritesFile();
    file.getParentDirectory().createDirectory();

    juce::StringArray lines;
    for (const auto &entry : entries)
        if (entry.favourite)
            lines.add(entry.name);

    file.replaceWithText(lines.joinIntoString("\n"));
}

void MixerView::PresetBrowserPopup::rebuildEntries()
{
    entries.clear();

    static const char *factoryNames[] = {"Balanced", "Punch", "Room", "Tight",
                                         "Studio Glue", "Punch Bus", "Tape Room", "Modern Tight"};

    for (int i = 0; i < static_cast<int>(std::size(factoryNames)); ++i)
        entries.push_back({factoryNames[i], {}, i, false});

    const auto dir = processor.getSettings().getConfigDirectory().getChildFile("mix_presets");
    if (dir.isDirectory())
    {
        const auto files = dir.findChildFiles(juce::File::findFiles, false, "*.glorpmix");
        for (const auto &file : files)
        {
            MixerPreset preset;
            if (!MixerPreset::fromXmlString(file.loadFileAsString(), preset))
                continue;

            entries.push_back({preset.name.isEmpty() ? file.getFileNameWithoutExtension() : preset.name,
                               file.getFullPathName(),
                               -1,
                               false});
        }
    }

    loadFavourites();
}

void MixerView::PresetBrowserPopup::rebuildRows()
{
    rowButtons.clear();

    const int rowH = 27;
    const int rowGap = 2;
    const int width = juce::jmax(120, listViewport.getWidth() - 12);

    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        const auto &entry = entries[static_cast<size_t>(i)];
        auto button = std::make_unique<HandButton>(
            (entry.favourite ? juce::String("* ") : juce::String("  ")) + entry.name, false);
        button->setTheme(theme);
        button->setActive(i == selectedEntry);
        button->onClick = [this, i]()
        {
            selectedEntry = i;
            for (size_t row = 0; row < rowButtons.size(); ++row)
                rowButtons[row]->setActive(static_cast<int>(row) == i);
            applyEntry(i);
        };
        button->setBounds(4, 4 + i * (rowH + rowGap), width, rowH);
        listContent.addAndMakeVisible(*button);
        rowButtons.push_back(std::move(button));
    }

    listContent.setSize(width + 8, juce::jmax(1, 8 + static_cast<int>(entries.size()) * (rowH + rowGap)));
    repaint();
}

void MixerView::PresetBrowserPopup::applyEntry(int index)
{
    if (index < 0 || index >= static_cast<int>(entries.size()))
        return;

    selectedEntry = index;
    const auto &entry = entries[static_cast<size_t>(index)];

    if (entry.factoryIndex >= 0)
    {
        owner.applyPreset(entry.factoryIndex);
    }
    else
    {
        MixerPreset preset;
        if (!MixerPreset::fromXmlString(juce::File(entry.filePath).loadFileAsString(), preset))
            return;

        owner.activePresetName = preset.name;
        processor.getSampler().applyMixerPreset(preset, true);

        for (auto &strip : owner.channelStrips)
            strip->updateFromState();
        if (owner.masterStrip != nullptr)
            owner.masterStrip->updateFromState();
        owner.repaint();
    }

    repaint();
}

void MixerView::PresetBrowserPopup::storeCurrentInActiveSlot()
{
    auto snapshot = processor.getSampler().getCurrentMixerPreset(owner.activePresetName);
    if (activeSlot == 0)
        slotA = std::move(snapshot);
    else
        slotB = std::move(snapshot);

    slotABtn.setActive(activeSlot == 0);
    slotBBtn.setActive(activeSlot == 1);
    repaint();
}

void MixerView::PresetBrowserPopup::recallSlot(int slotIndex)
{
    const auto &preset = slotIndex == 0 ? slotA : slotB;
    if (preset.micStrips.empty())
        return;

    activeSlot = slotIndex;
    owner.activePresetName = preset.name;
    processor.getSampler().applyMixerPreset(preset);

    for (auto &strip : owner.channelStrips)
        strip->updateFromState();
    if (owner.masterStrip != nullptr)
        owner.masterStrip->updateFromState();

    slotABtn.setActive(activeSlot == 0);
    slotBBtn.setActive(activeSlot == 1);
    owner.repaint();
    repaint();
}

void MixerView::PresetBrowserPopup::open()
{
    rebuildEntries();

    setVisible(true);
    toFront(true);
    resized();
    rebuildRows();

    const auto current = processor.getSampler().getCurrentMixerPreset(owner.activePresetName);
    slotA = current;
    slotB = current;
    activeSlot = 0;
    slotABtn.setActive(true);
    slotBBtn.setActive(false);

    repaint();
}

void MixerView::PresetBrowserPopup::close()
{
    setVisible(false);
}

void MixerView::PresetBrowserPopup::resized()
{
    closeBtn.setBounds(getWidth() - 86, 12, 72, 24);

    listViewport.setBounds(14, 52, getWidth() - 28, getHeight() - 144);

    const int y = getHeight() - 76;
    favouriteBtn.setBounds(16, y, 104, 26);
    slotABtn.setBounds(132, y, 38, 26);
    slotBBtn.setBounds(174, y, 38, 26);
    storeABtn.setBounds(222, y, 82, 26);
    storeBBtn.setBounds(310, y, 82, 26);
}

void MixerView::PresetBrowserPopup::setTheme(const Theme &t)
{
    theme = t;
    closeBtn.setTheme(t);
    favouriteBtn.setTheme(t);
    slotABtn.setTheme(t);
    slotBBtn.setTheme(t);
    storeABtn.setTheme(t);
    storeBBtn.setTheme(t);
    for (auto &row : rowButtons)
        row->setTheme(t);
    repaint();
}

void MixerView::PresetBrowserPopup::paint(juce::Graphics &g)
{
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 8.0f);
    drawSketchRect(g, r, theme.panelEdge, 2.2f, 211);

    auto headerR = juce::Rectangle<float>(r.getX() + 4.0f, r.getY() + 4.0f, r.getWidth() - 8.0f, 34.0f);
    g.setColour(theme.isDark ? juce::Colour(0xff3a3c44) : juce::Colour(0xffdfe6e9));
    g.fillRoundedRectangle(headerR, 6.0f);
    drawSketchRect(g, headerR, theme.isDark ? theme.panelEdge : theme.ink, 1.4f, 212);

    g.setColour(theme.isDark ? juce::Colours::white : theme.ink);
    g.setFont(displayFont(15.0f));
    g.drawText("PRESET BROWSER", 24, 12, getWidth() - 120, 26, juce::Justification::left, false);

    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.0f));
    g.drawText("FACTORY & USER PRESETS", 16, 42, getWidth() - 32, 10, juce::Justification::left, false);
    g.drawText("A/B COMPARE", 16, getHeight() - 88, getWidth() - 32, 10, juce::Justification::left, false);

    g.setColour(theme.panelEdge);
    g.drawHorizontalLine(getHeight() - 92, 16.0f, static_cast<float>(getWidth() - 16));
}

void MixerView::PresetBrowserPopup::mouseDown(const juce::MouseEvent &e)
{
    juce::ignoreUnused(e);
}

MixerView::BleedMatrixPopup::BleedMatrixPopup(MixerView &o, PluginProcessor &proc) : owner(o), processor(proc)
{
    closeBtn.onClick = [this]() { close(); };
    addAndMakeVisible(closeBtn);

    // Preset buttons
    naturalBtn.setButtonText("NATURAL");
    naturalBtn.onClick = [this]()
    {
        processor.getSampler().getBleedMatrix().resetToNatural();
        updateFromState();
    };
    addAndMakeVisible(naturalBtn);

    cleanBtn.setButtonText("CLEAN (DRY)");
    cleanBtn.onClick = [this]()
    {
        processor.getSampler().getBleedMatrix().resetToClean();
        updateFromState();
    };
    addAndMakeVisible(cleanBtn);

    liveBtn.setButtonText("LIVE ROOM");
    liveBtn.onClick = [this]()
    {
        processor.getSampler().getBleedMatrix().resetToLive();
        updateFromState();
    };
    addAndMakeVisible(liveBtn);

    ohOnlyBtn.setButtonText("OH ONLY");
    ohOnlyBtn.onClick = [this]()
    {
        const int numMics = processor.getSampler().numMicStrips();
        std::vector<std::string> names;
        for (int i = 0; i < numMics; ++i)
            names.push_back(processor.getSampler().micStripTitle(i).toStdString());
        processor.getSampler().getBleedMatrix().resetToOverheadsOnly(names);
        updateFromState();
    };
    addAndMakeVisible(ohOnlyBtn);

    // Modes
    hybridModeBtn.setButtonText("HYBRID ACOUSTIC");
    hybridModeBtn.activeColour = juce::Colour(0xff2ecc71);
    hybridModeBtn.onClick = [this]()
    {
        processor.getSampler().getBleedMatrix().setMode(BleedMatrix::Mode::hybridAcoustic);
        updateFromState();
    };
    addAndMakeVisible(hybridModeBtn);

    recordedModeBtn.setButtonText("RECORDED ONLY");
    recordedModeBtn.activeColour = juce::Colour(0xff3498db);
    recordedModeBtn.onClick = [this]()
    {
        processor.getSampler().getBleedMatrix().setMode(BleedMatrix::Mode::recordedOnly);
        updateFromState();
    };
    addAndMakeVisible(recordedModeBtn);

    disabledModeBtn.setButtonText("MUTE BLEED");
    disabledModeBtn.activeColour = juce::Colour(0xffe74c3c);
    disabledModeBtn.onClick = [this]()
    {
        processor.getSampler().getBleedMatrix().setMode(BleedMatrix::Mode::disabled);
        updateFromState();
    };
    addAndMakeVisible(disabledModeBtn);

    masterBleedKnob = std::make_unique<Knob>("BLEED LEVEL", 0.0f, 2.0f, 1.0f, "x");
    masterBleedKnob->onValueChanged = [this](float v)
    {
        processor.getSampler().getBleedMatrix().setMasterBleed(v);
        repaint();
    };
    addAndMakeVisible(*masterBleedKnob);

    sympResonanceKnob = std::make_unique<Knob>("SNARE BUZZ", 0.0f, 1.0f, 0.35f, "");
    sympResonanceKnob->onValueChanged = [this](float v)
    {
        processor.getSampler().getBleedMatrix().setSympatheticResonance(v);
        repaint();
    };
    addAndMakeVisible(*sympResonanceKnob);

    updateFromState();
    setTheme(owner.theme);
}

void MixerView::BleedMatrixPopup::open(int highlightMic)
{
    highlightedMicIndex = highlightMic;
    updateFromState();
    owner.setBleedMatrixOpen(true);
    repaint();
}

void MixerView::BleedMatrixPopup::close()
{
    highlightedMicIndex = -1;
    owner.setBleedMatrixOpen(false);
}

void MixerView::BleedMatrixPopup::updateFromState()
{
    const auto &bm = processor.getSampler().getBleedMatrix();
    if (masterBleedKnob)
        masterBleedKnob->setValue(bm.getMasterBleed());
    if (sympResonanceKnob)
        sympResonanceKnob->setValue(bm.getSympatheticResonance());

    hybridModeBtn.setActive(bm.getMode() == BleedMatrix::Mode::hybridAcoustic);
    recordedModeBtn.setActive(bm.getMode() == BleedMatrix::Mode::recordedOnly);
    disabledModeBtn.setActive(bm.getMode() == BleedMatrix::Mode::disabled);

    repaint();
}

void MixerView::BleedMatrixPopup::setTheme(const Theme &t)
{
    theme = t;
    closeBtn.setTheme(t);
    naturalBtn.setTheme(t);
    cleanBtn.setTheme(t);
    liveBtn.setTheme(t);
    ohOnlyBtn.setTheme(t);
    hybridModeBtn.setTheme(t);
    recordedModeBtn.setTheme(t);
    disabledModeBtn.setTheme(t);
    if (masterBleedKnob)
        masterBleedKnob->setTheme(t);
    if (sympResonanceKnob)
        sympResonanceKnob->setTheme(t);
    repaint();
}

void MixerView::BleedMatrixPopup::resized()
{
    closeBtn.setBounds(getWidth() - 76, 6, 68, 22);

    const int topBtnY = 32;
    naturalBtn.setBounds(12, topBtnY, 68, 22);
    cleanBtn.setBounds(84, topBtnY, 82, 22);
    liveBtn.setBounds(170, topBtnY, 78, 22);
    ohOnlyBtn.setBounds(252, topBtnY, 68, 22);

    hybridModeBtn.setBounds(326, topBtnY, 116, 22);
    recordedModeBtn.setBounds(446, topBtnY, 108, 22);
    disabledModeBtn.setBounds(558, topBtnY, 94, 22);

    if (masterBleedKnob)
        masterBleedKnob->setBounds(getWidth() - 146, 24, 52, 52);
    if (sympResonanceKnob)
        sympResonanceKnob->setBounds(getWidth() - 76, 24, 52, 52);

    gridBounds = juce::Rectangle<int>(12, 80, getWidth() - 24, getHeight() - 102);

    const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
    headerColWidth = 104;
    colWidth = (numMics > 0) ? std::max(60, (gridBounds.getWidth() - headerColWidth) / numMics) : 70;
    rowHeight = std::clamp((gridBounds.getHeight() - 2) / (BleedMatrix::maxPieces + 1), 16, 24);
}

bool MixerView::BleedMatrixPopup::getCellAtPos(juce::Point<int> pos, int &outPiece, int &outMic) const
{
    const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
    if (numMics <= 0)
        return false;

    const int x0 = gridBounds.getX() + headerColWidth;
    const int y0 = gridBounds.getY() + rowHeight;

    if (pos.x < x0 || pos.y < y0)
        return false;

    const int c = (pos.x - x0) / colWidth;
    const int r = (pos.y - y0) / rowHeight;

    if (c >= 0 && c < numMics && r >= 0 && r < BleedMatrix::maxPieces)
    {
        outPiece = r;
        outMic = c;
        return true;
    }
    return false;
}

bool MixerView::BleedMatrixPopup::getRowMuteAtPos(juce::Point<int> pos, int &outPiece) const
{
    const int startX = gridBounds.getX();
    const int startY = gridBounds.getY();
    const int y0 = startY + rowHeight;

    if (pos.x < startX || pos.x > startX + headerColWidth || pos.y < y0)
        return false;

    const int r = (pos.y - y0) / rowHeight;
    if (r >= 0 && r < BleedMatrix::maxPieces)
    {
        // [M] button sits in the rightmost 20px of the row header cell
        if (pos.x >= (startX + headerColWidth - 22) && pos.x <= (startX + headerColWidth - 4))
        {
            outPiece = r;
            return true;
        }
    }
    return false;
}

bool MixerView::BleedMatrixPopup::getColMuteAtPos(juce::Point<int> pos, int &outMic) const
{
    const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
    if (numMics <= 0)
        return false;

    const int startX = gridBounds.getX() + headerColWidth;
    const int startY = gridBounds.getY();

    if (pos.x < startX || pos.y < startY || pos.y > startY + rowHeight)
        return false;

    const int c = (pos.x - startX) / colWidth;
    if (c >= 0 && c < numMics)
    {
        const int colX = startX + c * colWidth;
        // [M] button sits in the rightmost 20px of the col header cell
        if (pos.x >= (colX + colWidth - 20) && pos.x <= (colX + colWidth - 2))
        {
            outMic = c;
            return true;
        }
    }
    return false;
}

void MixerView::BleedMatrixPopup::paint(juce::Graphics &g)
{
    const auto r = getLocalBounds().toFloat().reduced(1.0f);
    const auto ink = theme.ink;

    // Drop shadow (light mode)
    if (!theme.isDark)
    {
        g.setColour(ink.withAlpha(0.30f));
        g.fillRoundedRectangle(r.translated(3.0f, 3.0f), 6.0f);
    }

    // Panel background
    g.setColour(theme.panel);
    g.fillRoundedRectangle(r, 6.0f);
    drawSketchRect(g, r, theme.panelEdge, 2.0f, 204);

    // Header banner
    auto headerR = juce::Rectangle<float>(r.getX() + 4.0f, r.getY() + 3.0f, r.getWidth() - 8.0f, 26.0f);
    g.setColour(theme.isDark ? juce::Colour(0xff34363d) : juce::Colour(0xffdfe6e9));
    g.fillRoundedRectangle(headerR, 5.0f);
    drawSketchRect(g, headerR, theme.isDark ? theme.panelEdge : ink, 1.4f, 205);

    g.setColour(theme.isDark ? juce::Colours::white : ink);
    g.setFont(displayFont(13.5f));
    g.drawText("ACOUSTIC BLEED & LEAKAGE MATRIX", 16, 7, 300, 18, juce::Justification::left, false);

    g.setColour(theme.ink.withAlpha(0.65f));
    g.setFont(displayFont(9.5f));
    g.drawText("Logical Pieces x Mic Channels Leakage Factor", 305, 8, 300, 16, juce::Justification::left, false);

    const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
    if (numMics <= 0)
    {
        g.setColour(theme.ink);
        g.setFont(displayFont(13.0f));
        g.drawText("(Load a multi-microphone drum kit to view and tweak acoustic bleed)",
                   getLocalBounds(),
                   juce::Justification::centred,
                   false);
        return;
    }

    auto &bm = processor.getSampler().getBleedMatrix();

    // Table Header Row
    const int startX = gridBounds.getX();
    const int startY = gridBounds.getY();

    // Corner cell
    auto cornerR = juce::Rectangle<float>(static_cast<float>(startX),
                                          static_cast<float>(startY),
                                          static_cast<float>(headerColWidth),
                                          static_cast<float>(rowHeight));
    g.setColour(theme.trough);
    g.fillRoundedRectangle(cornerR, 3.0f);
    drawSketchRect(g, cornerR, theme.panelEdge, 1.2f, 301);
    g.setColour(theme.ink);
    g.setFont(displayFont(9.5f));
    g.drawText("PIECE \\ MIC", cornerR, juce::Justification::centred, false);

    // Column Headers (Mic names with channel accent color matching top strips)
    for (int m = 0; m < numMics; ++m)
    {
        auto colR = juce::Rectangle<float>(static_cast<float>(startX + headerColWidth + m * colWidth),
                                           static_cast<float>(startY),
                                           static_cast<float>(colWidth - 1),
                                           static_cast<float>(rowHeight));
        const bool isHighlighted = (m == highlightedMicIndex || m == hoveredMic);
        const auto micName = processor.getSampler().micStripTitle(m);
        const auto micAccent = getMicColor(micName);

        // Header background with strip accent tint
        g.setColour(isHighlighted ? micAccent.withAlpha(0.35f) : theme.trough);
        g.fillRoundedRectangle(colR, 3.0f);

        // Top accent strip matching the channel strip header above
        g.setColour(micAccent);
        g.fillRect(colR.getX(), colR.getY(), colR.getWidth(), 3.0f);

        drawSketchRect(g,
                       colR,
                       isHighlighted ? theme.accent : theme.panelEdge,
                       isHighlighted ? 1.6f : 1.1f,
                       static_cast<uint32_t>(310 + m));

        // Check if all cells in column are muted
        bool allColMuted = true;
        for (int p = 0; p < BleedMatrix::maxPieces; ++p)
        {
            if (!bm.isCellMuted(p, m))
            {
                allColMuted = false;
                break;
            }
        }

        // Column Mute button [M]
        auto colMuteR = juce::Rectangle<float>(
            colR.getRight() - 17.0f, colR.getY() + 4.0f, 14.0f, static_cast<float>(rowHeight - 6));
        if (allColMuted)
        {
            g.setColour(juce::Colour(0xffe74c3c));
            g.fillRoundedRectangle(colMuteR, 2.0f);
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setColour(hoveredColMuteMic == m ? theme.ink.withAlpha(0.25f) : theme.panel.withAlpha(0.4f));
            g.fillRoundedRectangle(colMuteR, 2.0f);
            g.setColour(theme.ink.withAlpha(0.7f));
        }
        g.setFont(displayFont(8.0f));
        g.drawText("M", colMuteR, juce::Justification::centred, false);

        // Column label
        auto textR = juce::Rectangle<int>(static_cast<int>(colR.getX() + 2.0f),
                                          static_cast<int>(colR.getY() + 2.0f),
                                          static_cast<int>(colR.getWidth() - 18.0f),
                                          static_cast<int>(colR.getHeight() - 2.0f));
        g.setColour(isHighlighted ? (theme.isDark ? juce::Colours::white : theme.ink) : theme.ink);
        g.setFont(displayFont(9.0f));
        g.drawFittedText(micName, textR, juce::Justification::centred, 1);
    }

    // Rows (9 logical pieces)
    for (int p = 0; p < BleedMatrix::maxPieces; ++p)
    {
        const int rowY = startY + (p + 1) * rowHeight;
        const auto pieceEnum = static_cast<BleedPiece>(p);

        // Row Header (Piece vector icon + name + macro mute)
        auto rowHdrR = juce::Rectangle<float>(static_cast<float>(startX),
                                              static_cast<float>(rowY),
                                              static_cast<float>(headerColWidth),
                                              static_cast<float>(rowHeight - 1));
        const bool isRowHovered = (hoveredPiece == p);
        g.setColour(isRowHovered ? theme.trough.brighter(0.08f) : theme.trough);
        g.fillRoundedRectangle(rowHdrR, 3.0f);
        drawSketchRect(g, rowHdrR, theme.panelEdge, 1.1f, static_cast<uint32_t>(400 + p));

        // Vector Icon on the left
        auto iconR = juce::Rectangle<float>(rowHdrR.getX() + 3.0f,
                                            rowHdrR.getY() + 2.0f,
                                            static_cast<float>(rowHeight - 5),
                                            static_cast<float>(rowHeight - 5));
        drawPieceVectorIcon(g, pieceEnum, iconR, theme.ink, 1.3f);

        // Piece Name text
        auto nameR = juce::Rectangle<float>(rowHdrR.getX() + static_cast<float>(rowHeight),
                                            rowHdrR.getY(),
                                            rowHdrR.getWidth() - static_cast<float>(rowHeight) - 20.0f,
                                            rowHdrR.getHeight());
        g.setColour(theme.ink);
        g.setFont(displayFont(9.5f));
        g.drawText(bleedPieceDisplayName(pieceEnum), nameR, juce::Justification::centredLeft, false);

        // Check if all cells in row are muted
        bool allRowMuted = true;
        for (int m = 0; m < numMics; ++m)
        {
            if (!bm.isCellMuted(p, m))
            {
                allRowMuted = false;
                break;
            }
        }

        // Row Mute button [M]
        auto rowMuteR = juce::Rectangle<float>(
            rowHdrR.getRight() - 17.0f, rowHdrR.getY() + 3.0f, 14.0f, static_cast<float>(rowHeight - 6));
        if (allRowMuted)
        {
            g.setColour(juce::Colour(0xffe74c3c));
            g.fillRoundedRectangle(rowMuteR, 2.0f);
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setColour(hoveredRowMutePiece == p ? theme.ink.withAlpha(0.25f) : theme.panel.withAlpha(0.4f));
            g.fillRoundedRectangle(rowMuteR, 2.0f);
            g.setColour(theme.ink.withAlpha(0.7f));
        }
        g.setFont(displayFont(8.0f));
        g.drawText("M", rowMuteR, juce::Justification::centred, false);

        // Cells for this piece
        for (int m = 0; m < numMics; ++m)
        {
            auto cellR = juce::Rectangle<float>(static_cast<float>(startX + headerColWidth + m * colWidth),
                                                static_cast<float>(rowY),
                                                static_cast<float>(colWidth - 1),
                                                static_cast<float>(rowHeight - 1));
            const float cellGain = bm.getCellGain(p, m);
            const bool isMuted = bm.isCellMuted(p, m);
            const bool isCellHovered = (hoveredPiece == p && hoveredMic == m);
            const bool isHighlighted = (m == highlightedMicIndex) || isCellHovered;

            // Background base
            g.setColour(theme.panel.withAlpha(0.85f));
            g.fillRoundedRectangle(cellR, 2.0f);

            if (isMuted)
            {
                // Muted state: soft red fill + slash
                g.setColour(juce::Colour(0x28e74c3c));
                g.fillRoundedRectangle(cellR, 2.0f);
                g.setColour(juce::Colour(0x60e74c3c));
                g.drawLine(
                    cellR.getX() + 4.0f, cellR.getBottom() - 4.0f, cellR.getRight() - 4.0f, cellR.getY() + 4.0f, 1.2f);
            }
            else
            {
                // Heatmap fill bar: bipolar around unity 1.0f (0 dB)
                const float centerTick = cellR.getX() + cellR.getWidth() * 0.5f;
                if (cellGain > 1.02f)
                {
                    // Boost: extends to the right in warm amber
                    const float boostRatio = std::clamp((cellGain - 1.0f) / 1.0f, 0.0f, 1.0f);
                    const float barW = (cellR.getWidth() * 0.48f) * boostRatio;
                    auto barR = juce::Rectangle<float>(centerTick, cellR.getY() + 1.0f, barW, cellR.getHeight() - 2.0f);
                    g.setColour(juce::Colour(0x45f39c12));
                    g.fillRoundedRectangle(barR, 1.5f);
                }
                else if (cellGain < 0.98f)
                {
                    // Cut: extends to the left in cool cyan
                    const float cutRatio = std::clamp((1.0f - cellGain) / 1.0f, 0.0f, 1.0f);
                    const float barW = (cellR.getWidth() * 0.48f) * cutRatio;
                    auto barR =
                        juce::Rectangle<float>(centerTick - barW, cellR.getY() + 1.0f, barW, cellR.getHeight() - 2.0f);
                    g.setColour(juce::Colour(0x353498db));
                    g.fillRoundedRectangle(barR, 1.5f);
                }

                // Unity center tick
                g.setColour(theme.panelEdge.withAlpha(0.4f));
                g.drawVerticalLine(static_cast<int>(centerTick), cellR.getY() + 2.0f, cellR.getBottom() - 2.0f);
            }

            // Cell border
            drawSketchRect(g,
                           cellR,
                           isHighlighted ? theme.accent : theme.panelEdge,
                           isHighlighted ? 1.5f : 1.0f,
                           static_cast<uint32_t>(500 + p * 32 + m));

            // Cell Text
            g.setFont(displayFont(8.5f));
            if (isMuted)
            {
                g.setColour(juce::Colour(0xffe74c3c));
                g.drawText("MUTE", cellR, juce::Justification::centred, false);
            }
            else
            {
                g.setColour(cellGain > 1.05f ? juce::Colour(0xffd35400) : theme.ink);
                const float db = (cellGain <= 0.001f) ? -60.0f : linearToDb(cellGain);
                juce::String label =
                    (std::abs(db) < 0.05f) ? "0.0 dB" : ((db > 0.0f ? "+" : "") + juce::String(db, 1) + " dB");
                g.drawText(label, cellR, juce::Justification::centred, false);
            }
        }
    }

    // Floating Tooltip Bubble when dragging or wheeling a cell
    if (isEditingValue && draggingPiece >= 0 && draggingMic >= 0)
    {
        auto bubbleR = juce::Rectangle<float>(
            static_cast<float>(activeEditPos.x - 34), static_cast<float>(activeEditPos.y - 32), 68.0f, 22.0f);
        // Keep inside bounds
        bubbleR.setX(std::clamp(bubbleR.getX(), 4.0f, static_cast<float>(getWidth() - 74)));
        bubbleR.setY(std::max(4.0f, bubbleR.getY()));

        // Bubble drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRoundedRectangle(bubbleR.translated(2.0f, 2.0f), 5.0f);

        // Bubble body
        g.setColour(theme.isDark ? juce::Colour(0xff2d3436) : juce::Colour(0xff2c3e50));
        g.fillRoundedRectangle(bubbleR, 5.0f);
        drawSketchRect(g, bubbleR, theme.accent, 1.5f, 999);

        g.setColour(juce::Colours::white);
        g.setFont(displayFont(10.0f));
        juce::String tipText = (bm.isCellMuted(draggingPiece, draggingMic))
                                   ? "MUTED"
                                   : ((activeEditDb > 0.0f ? "+" : "") + juce::String(activeEditDb, 1) + " dB");
        g.drawText(tipText, bubbleR, juce::Justification::centred, false);
    }

    // Bottom Help Hint
    auto hintR = juce::Rectangle<float>(
        static_cast<float>(startX), static_cast<float>(getHeight() - 20), static_cast<float>(getWidth() - 24), 16.0f);
    g.setColour(theme.ink.withAlpha(0.6f));
    g.setFont(displayFont(9.0f));
    g.drawText("Drag cell up/down or scroll wheel | Right-click/Shift-click to toggle MUTE | Double-click to reset (0 "
               "dB) | [M] buttons for row/col mute",
               hintR,
               juce::Justification::centred,
               false);
}

void MixerView::BleedMatrixPopup::mouseDown(const juce::MouseEvent &e)
{
    auto &bm = processor.getSampler().getBleedMatrix();
    const auto pos = e.getPosition();

    // 1. Check Row Mute [M] click
    int pMute = -1;
    if (getRowMuteAtPos(pos, pMute))
    {
        const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
        bool anyUnmuted = false;
        for (int m = 0; m < numMics; ++m)
        {
            if (!bm.isCellMuted(pMute, m))
            {
                anyUnmuted = true;
                break;
            }
        }
        for (int m = 0; m < numMics; ++m)
            bm.setCellMute(pMute, m, anyUnmuted);

        repaint();
        return;
    }

    // 2. Check Col Mute [M] click
    int colMute = -1;
    if (getColMuteAtPos(pos, colMute))
    {
        bool anyUnmuted = false;
        for (int p = 0; p < BleedMatrix::maxPieces; ++p)
        {
            if (!bm.isCellMuted(p, colMute))
            {
                anyUnmuted = true;
                break;
            }
        }
        for (int p = 0; p < BleedMatrix::maxPieces; ++p)
            bm.setCellMute(p, colMute, anyUnmuted);

        repaint();
        return;
    }

    // 3. Check Cell click
    int p = -1, m = -1;
    if (getCellAtPos(pos, p, m))
    {
        if (e.mods.isRightButtonDown() || e.mods.isShiftDown() || e.mods.isAltDown())
        {
            bm.setCellMute(p, m, !bm.isCellMuted(p, m));
            repaint();
            return;
        }

        draggingPiece = p;
        draggingMic = m;
        dragStartGain = bm.getCellGain(p, m);
        dragStartPos = pos;
        isEditingValue = true;
        const float db = (dragStartGain <= 0.001f) ? -60.0f : linearToDb(dragStartGain);
        activeEditDb = db;
        activeEditPos = pos;
        repaint();
    }
}

void MixerView::BleedMatrixPopup::mouseDrag(const juce::MouseEvent &e)
{
    if (draggingPiece >= 0 && draggingMic >= 0)
    {
        const int deltaY = dragStartPos.y - e.y;
        const float newGain = std::clamp(dragStartGain + static_cast<float>(deltaY) * 0.015f, 0.0f, 2.0f);
        auto &bm = processor.getSampler().getBleedMatrix();
        bm.setCellGain(draggingPiece, draggingMic, newGain);
        if (bm.isCellMuted(draggingPiece, draggingMic) && newGain > 0.01f)
            bm.setCellMute(draggingPiece, draggingMic, false);

        activeEditDb = (newGain <= 0.001f) ? -60.0f : linearToDb(newGain);
        activeEditPos = e.getPosition();
        isEditingValue = true;
        repaint();
    }
}

void MixerView::BleedMatrixPopup::mouseUp(const juce::MouseEvent & /*e*/)
{
    draggingPiece = -1;
    draggingMic = -1;
    isEditingValue = false;
    repaint();
}

void MixerView::BleedMatrixPopup::mouseDoubleClick(const juce::MouseEvent &e)
{
    auto &bm = processor.getSampler().getBleedMatrix();
    const auto pos = e.getPosition();

    // Double-click row header -> reset row to 0 dB
    int pRow = -1;
    if (getRowMuteAtPos(pos, pRow)
        || (pos.x >= gridBounds.getX() && pos.x <= gridBounds.getX() + headerColWidth
            && pos.y >= gridBounds.getY() + rowHeight))
    {
        const int p = (pos.y - (gridBounds.getY() + rowHeight)) / rowHeight;
        if (p >= 0 && p < BleedMatrix::maxPieces)
        {
            const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
            for (int m = 0; m < numMics; ++m)
            {
                bm.setCellGain(p, m, 1.0f);
                bm.setCellMute(p, m, false);
            }
            repaint();
            return;
        }
    }

    // Double-click col header -> reset col to 0 dB
    int colMute = -1;
    if (getColMuteAtPos(pos, colMute)
        || (pos.y >= gridBounds.getY() && pos.y <= gridBounds.getY() + rowHeight
            && pos.x >= gridBounds.getX() + headerColWidth))
    {
        const int m = (pos.x - (gridBounds.getX() + headerColWidth)) / colWidth;
        const int numMics = std::min(processor.getSampler().numMicStrips(), 32);
        if (m >= 0 && m < numMics)
        {
            for (int p = 0; p < BleedMatrix::maxPieces; ++p)
            {
                bm.setCellGain(p, m, 1.0f);
                bm.setCellMute(p, m, false);
            }
            repaint();
            return;
        }
    }

    // Double-click cell -> reset cell to 0 dB
    int p = -1, m = -1;
    if (getCellAtPos(pos, p, m))
    {
        bm.setCellGain(p, m, 1.0f);
        bm.setCellMute(p, m, false);
        repaint();
    }
}

void MixerView::BleedMatrixPopup::mouseMove(const juce::MouseEvent &e)
{
    const auto pos = e.getPosition();
    int p = -1, m = -1;
    bool needRepaint = false;

    if (getRowMuteAtPos(pos, p))
    {
        if (hoveredRowMutePiece != p)
        {
            hoveredRowMutePiece = p;
            needRepaint = true;
        }
        hoveredColMuteMic = -1;
        hoveredPiece = -1;
        hoveredMic = -1;
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    else if (getColMuteAtPos(pos, m))
    {
        if (hoveredColMuteMic != m)
        {
            hoveredColMuteMic = m;
            needRepaint = true;
        }
        hoveredRowMutePiece = -1;
        hoveredPiece = -1;
        hoveredMic = -1;
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    else if (getCellAtPos(pos, p, m))
    {
        if (hoveredPiece != p || hoveredMic != m)
        {
            hoveredPiece = p;
            hoveredMic = m;
            needRepaint = true;
        }
        hoveredRowMutePiece = -1;
        hoveredColMuteMic = -1;
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        if (hoveredPiece != -1 || hoveredMic != -1 || hoveredRowMutePiece != -1 || hoveredColMuteMic != -1)
        {
            hoveredPiece = -1;
            hoveredMic = -1;
            hoveredRowMutePiece = -1;
            hoveredColMuteMic = -1;
            needRepaint = true;
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    if (needRepaint)
        repaint();
}

void MixerView::BleedMatrixPopup::mouseExit(const juce::MouseEvent & /*e*/)
{
    hoveredPiece = -1;
    hoveredMic = -1;
    hoveredRowMutePiece = -1;
    hoveredColMuteMic = -1;
    isEditingValue = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void MixerView::BleedMatrixPopup::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
{
    int p = -1, m = -1;
    if (getCellAtPos(e.getPosition(), p, m))
    {
        auto &bm = processor.getSampler().getBleedMatrix();
        const float currentGain = bm.getCellGain(p, m);
        const float delta = (wheel.deltaY > 0.0f ? 0.06f : -0.06f);
        const float newGain = std::clamp(currentGain + delta, 0.0f, 2.0f);
        bm.setCellGain(p, m, newGain);
        if (bm.isCellMuted(p, m) && newGain > 0.01f)
            bm.setCellMute(p, m, false);

        hoveredPiece = p;
        hoveredMic = m;
        draggingPiece = p;
        draggingMic = m;
        isEditingValue = true;
        activeEditDb = (newGain <= 0.001f) ? -60.0f : linearToDb(newGain);
        activeEditPos = e.getPosition();
        repaint();
    }
}

//==============================================================================
// MixerView
//==============================================================================

MixerView::MixerView(PluginProcessor &proc) : processor(proc)
{
    channelContainer = std::make_unique<juce::Component>();
    channelViewport.setViewedComponent(channelContainer.get(), false);
    channelViewport.setScrollBarsShown(false, true, false, false);
    addAndMakeVisible(channelViewport);

    masterStrip = std::make_unique<MasterStripComponent>(*this, processor);
    addAndMakeVisible(*masterStrip);

    // Preset buttons
    presetCleanBtn.onClick = [this]() { applyPreset(0); };
    presetRockBtn.onClick = [this]() { applyPreset(1); };
    presetWarmBtn.onClick = [this]() { applyPreset(2); };
    presetPunchBtn.onClick = [this]() { applyPreset(3); };
    presetDryBtn.onClick = [this]() { applyPreset(4); };
    presetMixBtn.onClick = [this]() { applyPreset(5); };

    savePresetBtn.onClick = [this]() { promptSavePreset(); };
    presetBrowserBtn.onClick = [this]()
    {
        if (presetBrowser != nullptr)
        {
            closePopups();
            presetBrowser->open();
        }
    };
    addAndMakeVisible(presetBrowserBtn);

    outputModeBtn.onClick = [this]
    {
        auto &sampler = processor.getSampler();
        const bool factory = sampler.isFactoryPieceMode();
        const auto mode = sampler.getOutputMode();

        juce::PopupMenu menu;
        menu.addSectionHeader("OUTPUT ROUTING");
        menu.addItem("Default Stereo (Mix Bus -> Out 1/2)", true, mode == OutputMode::stereo, [this]
        {
            processor.getSampler().setOutputMode(OutputMode::stereo);
            updateOutputModeButton();
            repaint();
        });
        menu.addItem("Multi-Out Easy (Sums only)", factory, mode == OutputMode::multiEasy, [this]
        {
            processor.getSampler().setOutputMode(OutputMode::multiEasy);
            updateOutputModeButton();
            repaint();
        });
        menu.addItem("Multi-Out Advanced (Groups)", factory, mode == OutputMode::multiAdvanced, [this]
        {
            processor.getSampler().setOutputMode(OutputMode::multiAdvanced);
            updateOutputModeButton();
            repaint();
        });
        menu.addItem("Full Multi-Out (Every piece)", factory, mode == OutputMode::full, [this]
        {
            processor.getSampler().setOutputMode(OutputMode::full);
            updateOutputModeButton();
            repaint();
        });

        juce::PopupMenu::Options opts;
        opts = opts.withTargetComponent(&outputModeBtn).withParentComponent(this);
        menu.showMenuAsync(opts);
    };
    addAndMakeVisible(outputModeBtn);

    // Generic factory presets from Suite Drums Glorp are intentionally not
    // shown; this bundle will get configuration-aware mix presets later.

    bleedMatrixBtn.setButtonText("BLEED MATRIX");
    bleedMatrixBtn.activeColour = juce::Colour(0xff4cc2a8);
    bleedMatrixBtn.onClick = [this]() { toggleBleedMatrix(); };
    addAndMakeVisible(bleedMatrixBtn);

    channelFXPopup = std::make_unique<ChannelFXPopup>(*this, processor);
    insertRackPopup = std::make_unique<InsertRackPopup>(*this, processor);
    masterFXPopup = std::make_unique<MasterFXPopup>(*this, processor);
    presetBrowser = std::make_unique<PresetBrowserPopup>(*this, processor);
    bleedMatrixPopup = std::make_unique<BleedMatrixPopup>(*this, processor);
    addChildComponent(*channelFXPopup);
    addChildComponent(*insertRackPopup);
    addChildComponent(*masterFXPopup);
    addChildComponent(*presetBrowser);
    addChildComponent(*bleedMatrixPopup);

    refreshChannels();
    startTimerHz(30); // 30Hz meter redraw
}

MixerView::~MixerView()
{
    stopTimer();
}

void MixerView::refreshChannels()
{
    // A kit change invalidates every strip index, so drop any popup that was
    // editing one of them before rebuilding the channel list.
    closePopups();

    channelStrips.clear();
    const int numMics = processor.getSampler().numMicStrips();

    for (int i = 0; i < numMics; ++i)
    {
        auto strip = std::make_unique<ChannelStripComponent>(*this, processor, i);
        strip->setTheme(theme);
        channelContainer->addAndMakeVisible(*strip);
        channelStrips.push_back(std::move(strip));
    }

    busStrips.clear();
    const int numBuses = processor.getSampler().numFactoryBuses();
    for (int i = 0; i < numBuses; ++i)
    {
        auto strip = std::make_unique<BusStripComponent>(*this, processor, i);
        strip->setTheme(theme);
        channelContainer->addAndMakeVisible(*strip);
        busStrips.push_back(std::move(strip));
    }

    if (masterStrip != nullptr)
        masterStrip->updateFromState();

    // Factory kits mix pieces, not microphones, so the mic bleed matrix does
    // not apply. Polarity/pan are already baked into each piece during assembly.
    bleedMatrixBtn.setVisible(false);
    updateOutputModeButton();
    if (bleedMatrixOpen)
        setBleedMatrixOpen(false);

    resized();
    repaint();
}

void MixerView::refreshBusStates()
{
    for (auto &strip : channelStrips)
        strip->updateFromState();

    if (masterStrip != nullptr)
        masterStrip->updateFromState();
}

void MixerView::focusChannel(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channelStrips.size()))
        return;

    auto *strip = channelStrips[static_cast<size_t>(channelIndex)].get();
    const int maxScroll = std::max(0, channelContainer->getWidth() - channelViewport.getWidth());
    channelViewport.setViewPosition(juce::jlimit(0, maxScroll, strip->getX() - 40), 0);
    openChannelFX(channelIndex);
}

void MixerView::openChannelFX(int channelIndex)
{
    if (insertRackPopup != nullptr)
    {
        closePopups();
        insertRackPopup->open(false, channelIndex);
    }
}

void MixerView::openMasterFX()
{
    if (masterFXPopup != nullptr)
        masterFXPopup->open();
}

void MixerView::setBleedMatrixOpen(bool open)
{
    if (bleedMatrixOpen == open)
        return;

    bleedMatrixOpen = open;
    bleedMatrixBtn.setActive(bleedMatrixOpen);

    if (bleedMatrixPopup != nullptr)
    {
        bleedMatrixPopup->setVisible(bleedMatrixOpen);
        if (bleedMatrixOpen)
            bleedMatrixPopup->updateFromState();
    }

    resized();
    repaint();
}

void MixerView::toggleBleedMatrix()
{
    setBleedMatrixOpen(!bleedMatrixOpen);
}

void MixerView::openInsertRack(bool bus, int channelIndex, int slot)
{
    closePopups();

    if (insertRackPopup != nullptr)
        insertRackPopup->open(bus, channelIndex, slot);
}

void MixerView::openBleedMatrix(int highlightMic)
{
    bleedMatrixOpen = true;
    bleedMatrixBtn.setActive(true);
    if (bleedMatrixPopup != nullptr)
        bleedMatrixPopup->open(highlightMic);
    resized();
    repaint();
}

void MixerView::closePopups()
{
    hideInsertPicker();

    if (channelFXPopup != nullptr && channelFXPopup->isOpen())
        channelFXPopup->close();

    if (insertRackPopup != nullptr && insertRackPopup->isOpen())
        insertRackPopup->close();

    if (masterFXPopup != nullptr && masterFXPopup->isOpen())
        masterFXPopup->close();

    if (presetBrowser != nullptr && presetBrowser->isOpen())
        presetBrowser->close();
}

void MixerView::showInsertMenu(bool bus, int channelIndex, int slot, juce::Component *target,
                               juce::Rectangle<int> area)
{
    hideInsertPicker();

    const int panelW = 196;
    const int panelH = 6 * 30 + 16;

    juce::Component::SafePointer<MixerView> safeThis(this);
    auto panel = std::make_unique<InsertTypePicker>(
        theme,
        [this, safeThis, bus, channelIndex, slot](InsertType type)
        {
            processor.getSampler().setInsertType(bus, channelIndex, slot, type);
            if (insertRackPopup != nullptr && insertRackPopup->isOpen())
                insertRackPopup->open(bus, channelIndex, slot);
            for (auto &strip : channelStrips)
                if (strip != nullptr)
                    strip->repaint();
            for (auto &strip : busStrips)
                if (strip != nullptr)
                    strip->repaint();

            juce::MessageManager::callAsync([safeThis]
                                            {
                                                if (safeThis != nullptr)
                                                    safeThis->hideInsertPicker();
                                            });
        });

    const auto areaInMixer = target != nullptr ? getLocalArea(target, area) : juce::Rectangle<int>(0, 0, 1, 1);

    // Prefer directly below the clicked row; flip above when there is no room.
    int px = juce::jlimit(4, juce::jmax(4, getWidth() - panelW - 4), areaInMixer.getX());
    int py = areaInMixer.getBottom() + 6;
    if (py + panelH > getHeight() - 4)
        py = areaInMixer.getY() - panelH - 6;
    py = juce::jlimit(4, juce::jmax(4, getHeight() - panelH - 4), py);
    panel->setBounds(px, py, panelW, panelH);
    insertPickerPanel = std::move(panel);

    auto layer = std::make_unique<InsertPickerLayer>([safeThis]
                                                    {
                                                        if (safeThis != nullptr)
                                                            safeThis->hideInsertPicker();
                                                    });
    layer->setBounds(getLocalBounds());
    layer->addAndMakeVisible(*insertPickerPanel);
    insertPickerLayer = std::move(layer);

    addAndMakeVisible(*insertPickerLayer);
    insertPickerLayer->toFront(true);
}

void MixerView::hideInsertPicker()
{
    if (insertPickerLayer != nullptr)
    {
        if (insertPickerPanel != nullptr)
            insertPickerLayer->removeChildComponent(insertPickerPanel.get());
        removeChildComponent(insertPickerLayer.get());
    }

    insertPickerPanel.reset();
    insertPickerLayer.reset();
}

void MixerView::updateOutputModeButton()
{
    const bool factory = processor.getSampler().isFactoryPieceMode();
    const auto mode = processor.getSampler().getOutputMode();

    juce::String label = "OUT: STEREO";
    switch (mode)
    {
        case OutputMode::multiEasy: label = "OUT: SUMS"; break;
        case OutputMode::multiAdvanced: label = "OUT: GROUPS"; break;
        case OutputMode::full: label = "OUT: FULL"; break;
        case OutputMode::stereo:
        default: label = "OUT: STEREO"; break;
    }

    outputModeBtn.setButtonText(label);
    outputModeBtn.setActive(mode != OutputMode::stereo && factory);
    outputModeBtn.setEnabled(factory);
}

void MixerView::applyPreset(int presetIndex)
{
    presetIndex = juce::jlimit(0, 7, presetIndex);

    const auto pieces = processor.getMixPieces();
    const auto presetName = MixerPreset::configurationMixName(presetIndex);

    processor.getSampler().applyFactoryMix(presetIndex, pieces);
    activePresetName = presetName;

    for (auto &s : channelStrips)
        s->updateFromState();

    refreshBusStates();

    if (masterStrip != nullptr)
        masterStrip->updateFromState();

    repaint();
}

void MixerView::promptSavePreset()
{
    const auto dir = processor.getSettings().getConfigDirectory().getChildFile("mix_presets");
    dir.createDirectory();

    auto file = dir.getChildFile(activePresetName.toLowerCase().replace(" ", "_") + ".glorpmix");
    const auto preset = processor.getSampler().getCurrentMixerPreset(activePresetName);
    file.replaceWithText(preset.toXmlString());
}

void MixerView::ChannelStripComponent::repaintMeter()
{
    const auto &sampler = processor.getSampler();
    const float peak = std::max(sampler.getMicPeakL(channelIndex_), sampler.getMicPeakR(channelIndex_));
    if (std::abs(peak - lastDisplayedPeak) > 0.002f || (peak == 0.0f && lastDisplayedPeak > 0.0f))
    {
        lastDisplayedPeak = peak;
        if (!meterBounds.isEmpty())
            repaint(meterBounds.expanded(2.0f).getSmallestIntegerContainer());
        else
            repaint();
    }
}

void MixerView::MasterStripComponent::repaintMeter()
{
    const auto &sampler = processor.getSampler();
    const float peakL = sampler.getMasterPeakL();
    const float peakR = sampler.getMasterPeakR();
    if (std::abs(peakL - lastDisplayedPeakL) > 0.002f || std::abs(peakR - lastDisplayedPeakR) > 0.002f
        || ((peakL == 0.0f && peakR == 0.0f) && (lastDisplayedPeakL > 0.0f || lastDisplayedPeakR > 0.0f)))
    {
        lastDisplayedPeakL = peakL;
        lastDisplayedPeakR = peakR;
        if (!meterLBounds.isEmpty() && !meterRBounds.isEmpty())
            repaint(meterLBounds.getUnion(meterRBounds).expanded(2.0f).getSmallestIntegerContainer());
        else
            repaint();
    }
}

void MixerView::timerCallback()
{
    if (!isVisible())
        return;

    // Repaint only meter regions whose peak values changed
    for (auto &s : channelStrips)
        s->repaintMeter();

    for (auto &s : busStrips)
        if (s != nullptr)
            s->repaint();

    if (masterStrip != nullptr)
        masterStrip->repaintMeter();
}

void MixerView::resized()
{
    if (insertPickerLayer != nullptr)
    {
        juce::Component::SafePointer<MixerView> safeThis(this);
        juce::MessageManager::callAsync([safeThis]
                                        {
                                            if (safeThis != nullptr)
                                                safeThis->hideInsertPicker();
                                        });
    }

    const auto r = getLocalBounds().toFloat();

    // Top control bar
    const float btnY = 10.0f;
    const float btnH = 26.0f;
    presetBrowserBtn.setBounds(16, btnY, 96, btnH);
    outputModeBtn.setBounds(120, btnY, 122, btnH);
    bleedMatrixBtn.setBounds(250, btnY, 114, btnH);

    // Channel Area vs Master Strip
    const float contentY = 46.0f;
    const float totalAvailH = r.getHeight() - contentY - 8.0f;
    const float masterW = 100.0f;
    const float viewW = r.getWidth() - masterW - 20.0f;

    float contentH = totalAvailH;
    if (bleedMatrixOpen)
    {
        contentH = std::round(totalAvailH * 0.44f);
        contentH = std::clamp(contentH, 205.0f, 255.0f);
    }

    channelViewport.setBounds(12, static_cast<int>(contentY), static_cast<int>(viewW), static_cast<int>(contentH));

    const float stripW = 100.0f;
    const float busStripW = 112.0f;
    const float gap = 6.0f;
    const float busGap = 10.0f;
    const float totalStripsW = static_cast<float>(channelStrips.size()) * (stripW + gap)
                               + (busStrips.empty() ? 0.0f
                                                    : busGap + static_cast<float>(busStrips.size()) * (busStripW + gap));
    const float containerW = std::max(viewW, totalStripsW + 12.0f);

    // The horizontal scrollbar lives inside the viewport, so strips drawn at
    // the full viewport height lose their bottom edge underneath it. Reserve
    // the bar's thickness whenever the channel row actually overflows.
    const bool needsHorizontalScroll = containerW > viewW + 0.5f;
    const float stripH = contentH - (needsHorizontalScroll
                                          ? static_cast<float>(channelViewport.getScrollBarThickness())
                                          : 0.0f);

    channelContainer->setBounds(0, 0, static_cast<int>(containerW), static_cast<int>(stripH));

    float curX = 4.0f;
    for (auto &s : channelStrips)
    {
        s->setBounds(static_cast<int>(curX), 0, static_cast<int>(stripW), static_cast<int>(stripH));
        curX += stripW + gap;
    }

    if (!busStrips.empty())
    {
        curX += busGap - gap;
        for (auto &bus : busStrips)
        {
            bus->setBounds(static_cast<int>(curX), 0, static_cast<int>(busStripW), static_cast<int>(stripH));
            curX += busStripW + gap;
        }
    }

    masterStrip->setBounds(static_cast<int>(r.getWidth() - masterW - 8.0f),
                           static_cast<int>(contentY),
                           static_cast<int>(masterW),
                           static_cast<int>(stripH));

    if (channelFXPopup != nullptr)
        channelFXPopup->setBounds(50, 40, std::min(480, getWidth() - 100), 340);

    if (insertRackPopup != nullptr)
    {
        const int w = std::min(860, getWidth() - 60);
        const int h = std::min(390, getHeight() - 90);
        insertRackPopup->setBounds((getWidth() - w) / 2, 64, w, h);
    }

    if (masterFXPopup != nullptr)
        masterFXPopup->setBounds(std::max(8, getWidth() - 740), 40, 720, 360);

    if (presetBrowser != nullptr)
        presetBrowser->setBounds(std::max(8, getWidth() - 450), 40, 430, 430);

    if (bleedMatrixPopup != nullptr)
    {
        if (bleedMatrixOpen)
        {
            const float matrixY = contentY + contentH + 6.0f;
            const float matrixH = r.getHeight() - matrixY - 6.0f;
            bleedMatrixPopup->setBounds(
                12, static_cast<int>(matrixY), static_cast<int>(r.getWidth() - 24.0f), static_cast<int>(matrixH));
            bleedMatrixPopup->setVisible(true);
        }
        else
        {
            bleedMatrixPopup->setVisible(false);
        }
    }
}

void MixerView::setTheme(const Theme &newTheme)
{
    theme = newTheme;

    presetCleanBtn.setTheme(theme);
    presetRockBtn.setTheme(theme);
    presetWarmBtn.setTheme(theme);
    presetPunchBtn.setTheme(theme);
    presetDryBtn.setTheme(theme);
    presetMixBtn.setTheme(theme);
    savePresetBtn.setTheme(theme);
    presetBrowserBtn.setTheme(theme);
    outputModeBtn.setTheme(theme);
    if (presetBrowser != nullptr)
        presetBrowser->setTheme(theme);
    bleedMatrixBtn.setTheme(theme);

    for (auto &strip : channelStrips)
        if (strip != nullptr)
            strip->setTheme(theme);

    for (auto &strip : busStrips)
        if (strip != nullptr)
            strip->setTheme(theme);

    if (masterStrip != nullptr)
        masterStrip->setTheme(theme);

    if (channelFXPopup != nullptr)
        channelFXPopup->setTheme(theme);

    if (insertRackPopup != nullptr)
        insertRackPopup->setTheme(theme);

    if (masterFXPopup != nullptr)
        masterFXPopup->setTheme(theme);

    if (bleedMatrixPopup != nullptr)
        bleedMatrixPopup->setTheme(theme);

    repaint();
}

void MixerView::paint(juce::Graphics &g)
{
    // Background matching the plugin theme
    g.fillAll(theme.background);

    // Active preset sits in the gap directly above the master strip, not in the
    // top button row where it used to collide with BLEED MATRIX at narrow sizes.
    const int labelRight = masterStrip != nullptr ? masterStrip->getRight() : getWidth() - 8;
    const int labelWidth = 220;
    g.setColour(theme.inkSoft);
    g.setFont(uiFont(9.5f).boldened());
    g.drawFittedText("PRESET: " + activePresetName.toUpperCase(),
                     labelRight - labelWidth,
                     36,
                     labelWidth,
                     9,
                     juce::Justification::centredRight,
                     1,
                     0.78f);
}

void MixerView::mouseDown(const juce::MouseEvent &e)
{
    // Dismiss popups if clicking outside
    if (channelFXPopup && channelFXPopup->isOpen() && !channelFXPopup->getBounds().contains(e.position.toInt()))
        channelFXPopup->close();

    if (insertRackPopup && insertRackPopup->isOpen() && !insertRackPopup->getBounds().contains(e.position.toInt()))
        insertRackPopup->close();

    if (masterFXPopup && masterFXPopup->isOpen() && !masterFXPopup->getBounds().contains(e.position.toInt()))
        masterFXPopup->close();

    if (presetBrowser && presetBrowser->isOpen() && !presetBrowser->getBounds().contains(e.position.toInt()))
        presetBrowser->close();

    if (bleedMatrixPopup && bleedMatrixPopup->isOpen() && !bleedMatrixPopup->getBounds().contains(e.position.toInt()))
        bleedMatrixPopup->close();
}

} // namespace glorp
