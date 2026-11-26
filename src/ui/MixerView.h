/*
    MixerView.h - Full-featured per-mic drum mixer console and DSP rack for Suite Drums Glorp.
*/

#pragma once

#include "ChannelFX.h"
#include "Knob.h"
#include "PluginProcessor.h"
#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace glorp
{

class MixerView
    : public juce::Component
    , private juce::Timer
{
  public:
    explicit MixerView(PluginProcessor &processor);
    ~MixerView() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &e) override;

    void refreshChannels();
    void refreshBusStates();
    void focusChannel(int channelIndex);
    void openChannelFX(int channelIndex);
    void openInsertRack(bool bus, int channel, int slot);
    void openMasterFX();
    void openBleedMatrix(int highlightMic = -1);
    void toggleBleedMatrix();
    void setBleedMatrixOpen(bool open);
    bool isBleedMatrixOpen() const noexcept { return bleedMatrixOpen; }
    void closePopups();
    void setTheme(const Theme &newTheme);

  private:
    void timerCallback() override;

    PluginProcessor &processor;
    Theme theme = Theme::suite();
    bool bleedMatrixOpen = false;

    //==========================================================================
    // UI Helpers & Custom Buttons
    //==========================================================================
    class HandButton : public juce::Button
    {
      public:
        HandButton(const juce::String &name, bool isToggle = false);
        void paintButton(juce::Graphics &g, bool isHover, bool isDown) override;
        void setActive(bool a)
        {
            active = a;
            repaint();
        }
        bool getActive() const noexcept { return active; }
        void setTheme(const Theme &t)
        {
            theme = t;
            repaint();
        }
        juce::Colour activeColour = juce::Colour(0xfff1c40f);

      private:
        bool isToggle_ = false;
        bool active = false;
        Theme theme = Theme::suite();
    };

    //==========================================================================
    // Channel FX Drawer Popup (EQ, Comp, Saturation, Transient)
    //==========================================================================
    class ChannelFXPopup : public juce::Component
    {
      public:
        ChannelFXPopup(MixerView &owner, PluginProcessor &processor);
        void openForChannel(int channelIndex);
        void close();
        bool isOpen() const noexcept { return isVisible(); }
        void setTheme(const Theme &t);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;

      private:
        MixerView &owner;
        PluginProcessor &processor;
        int currentChannel = -1;
        Theme theme = Theme::suite();

        HandButton closeBtn {"CLOSE"};
        HandButton eqToggle {"EQ ON", true};
        HandButton compToggle {"COMP ON", true};

        // EQ Knobs
        std::unique_ptr<Knob> eqLowGainKnob;
        std::unique_ptr<Knob> eqMidGainKnob;
        std::unique_ptr<Knob> eqMidFreqKnob;
        std::unique_ptr<Knob> eqHighGainKnob;

        // Compressor Knobs
        std::unique_ptr<Knob> compThreshKnob;
        std::unique_ptr<Knob> compRatioKnob;
        std::unique_ptr<Knob> compAttackKnob;
        std::unique_ptr<Knob> compReleaseKnob;
        std::unique_ptr<Knob> compMakeupKnob;

        // Filters
        std::unique_ptr<Knob> hpfKnob;
        std::unique_ptr<Knob> lpfKnob;

        // Saturation & Transient Shaper Knobs
        std::unique_ptr<Knob> satDriveKnob;
        std::unique_ptr<Knob> transAttackKnob;
        std::unique_ptr<Knob> transSustainKnob;
    };

    //==========================================================================
    // Master FX Drawer Popup (Reverb Return, Size, Damping, Limiter)
    //==========================================================================
    class MasterFXPopup : public juce::Component
    {
      public:
        MasterFXPopup(MixerView &owner, PluginProcessor &processor);
        void open();
        void close();
        bool isOpen() const noexcept { return isVisible(); }
        void setTheme(const Theme &t);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;

      private:
        MixerView &owner;
        PluginProcessor &processor;
        Theme theme = Theme::suite();

        HandButton closeBtn {"CLOSE"};
        HandButton limiterToggle {"LIMITER", true};
        HandButton busCompToggle {"BUS COMP", false};

        std::unique_ptr<Knob> busLowKnob;
        std::unique_ptr<Knob> busMidKnob;
        std::unique_ptr<Knob> busMidFreqKnob;
        std::unique_ptr<Knob> busHighKnob;
        std::unique_ptr<Knob> busThreshKnob;
        std::unique_ptr<Knob> busRatioKnob;
        std::unique_ptr<Knob> busAttackKnob;
        std::unique_ptr<Knob> busReleaseKnob;
        std::unique_ptr<Knob> busMakeupKnob;
        std::unique_ptr<Knob> busMixKnob;
        std::unique_ptr<Knob> busSatKnob;

        std::unique_ptr<Knob> revReturnKnob;
        std::unique_ptr<Knob> revSizeKnob;
        std::unique_ptr<Knob> revDampKnob;
        std::unique_ptr<Knob> revWidthKnob;
    };

    //==========================================================================
    // Insert Rack Popup
    //==========================================================================
    class InsertRackPopup : public juce::Component, private juce::Timer
    {
      public:
        InsertRackPopup(MixerView &owner, PluginProcessor &processor);
        void open(bool bus, int channel, int slot = 0);
        void close();
        bool isOpen() const noexcept { return isVisible(); }
        void setTheme(const Theme &t);
        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseUp(const juce::MouseEvent &e) override;

      private:
        void timerCallback() override;
        void drawCompressorMeter(juce::Graphics &g, juce::Rectangle<float> area);
        void drawClipperCurve(juce::Graphics &g, juce::Rectangle<float> area);
        int eqNodeAt(juce::Point<float> pos, juce::Rectangle<float> plot) const;
        void moveEqNode(int band, juce::Point<float> pos, juce::Rectangle<float> plot);

        MixerView &owner;
        PluginProcessor &processor;
        Theme theme = Theme::suite();
        bool busTarget = false;
        int channel = 0;
        int selectedSlot = 0;
        int dragFrom = -1;
        int dragTo = -1;
        bool dragging = false;
        int draggingEqNode = -1;
        juce::Point<float> dragStart;

        juce::TextButton closeBtn {"CLOSE"};
        std::array<juce::Rectangle<float>, InsertChain::slotCount> slotBounds;
        std::array<juce::Rectangle<float>, InsertChain::slotCount> bypassBounds;
        std::array<juce::Rectangle<float>, InsertChain::slotCount> removeBounds;
        std::vector<std::unique_ptr<Knob>> knobs;
        std::vector<int> knobParams;
        juce::Rectangle<float> graphBounds;
        int selectedEqBand = 3;
        std::array<juce::Rectangle<float>, eqBandCount> eqChipBounds;
        juce::Rectangle<float> eqOnBounds;
        juce::Rectangle<float> eqSlopeBounds;

        void rebuildKnobs();
        void drawSlot(juce::Graphics &g, int slot, juce::Rectangle<float> area);
        void drawEqGraph(juce::Graphics &g, juce::Rectangle<float> area);
        void drawEqControls(juce::Graphics &g);
        void selectEqBand(int band);
        void setEqValue(int param, float value);
        void syncEqKnobs();
        int slotAt(juce::Point<float> pos) const;
        juce::String slotTypeLabel(int slot) const;
    };

    //==========================================================================
    // Preset Browser (factory + user presets, A/B compare, favourites)
    //==========================================================================
    // Preset Browser (factory + user presets, A/B compare, favourites)
    //==========================================================================
    class PresetBrowserPopup : public juce::Component
    {
      public:
        PresetBrowserPopup(MixerView &owner, PluginProcessor &processor);
        void open();
        void close();
        bool isOpen() const noexcept { return isVisible(); }
        void setTheme(const Theme &t);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;

      private:
        struct Entry
        {
            juce::String name;
            juce::String filePath;
            int factoryIndex = -1;
            bool favourite = false;
        };

        MixerView &owner;
        PluginProcessor &processor;
        Theme theme = Theme::suite();

        HandButton closeBtn {"CLOSE"};
        HandButton favouriteBtn {"FAVOURITE"};
        HandButton slotABtn {"A"};
        HandButton slotBBtn {"B"};
        HandButton storeABtn {"STORE A"};
        HandButton storeBBtn {"STORE B"};
        juce::Component listContent;
        juce::Viewport listViewport;
        std::vector<std::unique_ptr<HandButton>> rowButtons;
        std::vector<Entry> entries;
        int selectedEntry = -1;

        MixerPreset slotA;
        MixerPreset slotB;
        int activeSlot = 0;

        juce::File favouritesFile() const;
        void loadFavourites();
        void saveFavourites() const;
        void rebuildEntries();
        void rebuildRows();
        void applyEntry(int index);
        void storeCurrentInActiveSlot();
        void recallSlot(int slot);
    };

    //==========================================================================
    // Bleed Matrix Popup / Panel (Multi-mic Leakage & Acoustic Crosstalk Matrix)
    //==========================================================================
    class BleedMatrixPopup : public juce::Component
    {
      public:
        BleedMatrixPopup(MixerView &owner, PluginProcessor &processor);
        void open(int highlightMic = -1);
        void close();
        bool isOpen() const noexcept { return isVisible(); }
        void updateFromState();
        void setTheme(const Theme &t);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseUp(const juce::MouseEvent &e) override;
        void mouseDoubleClick(const juce::MouseEvent &e) override;
        void mouseMove(const juce::MouseEvent &e) override;
        void mouseExit(const juce::MouseEvent &e) override;
        void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

      private:
        MixerView &owner;
        PluginProcessor &processor;
        Theme theme = Theme::suite();
        int highlightedMicIndex = -1;

        HandButton closeBtn {"CLOSE"};

        // Presets
        HandButton naturalBtn {"NATURAL"};
        HandButton cleanBtn {"CLEAN (DRY)"};
        HandButton liveBtn {"LIVE ROOM"};
        HandButton ohOnlyBtn {"OH ONLY"};

        // Modes
        HandButton hybridModeBtn {"HYBRID ACOUSTIC", true};
        HandButton recordedModeBtn {"RECORDED ONLY", true};
        HandButton disabledModeBtn {"MUTE BLEED", true};

        std::unique_ptr<Knob> masterBleedKnob;
        std::unique_ptr<Knob> sympResonanceKnob;

        // Matrix Grid Interaction
        int draggingPiece = -1;
        int draggingMic = -1;
        float dragStartGain = 1.0f;
        juce::Point<int> dragStartPos;

        // Hover & Macro Mutes
        int hoveredPiece = -1;
        int hoveredMic = -1;
        int hoveredRowMutePiece = -1;
        int hoveredColMuteMic = -1;

        // Tooltip bubble
        bool isEditingValue = false;
        float activeEditDb = 0.0f;
        juce::Point<int> activeEditPos;

        // Cached layout
        juce::Rectangle<int> gridBounds;
        int rowHeight = 22;
        int colWidth = 64;
        int headerColWidth = 100;

        bool getCellAtPos(juce::Point<int> pos, int &outPiece, int &outMic) const;
        bool getRowMuteAtPos(juce::Point<int> pos, int &outPiece) const;
        bool getColMuteAtPos(juce::Point<int> pos, int &outMic) const;
    };

    //==========================================================================
    // Individual Channel Strip Component
    //==========================================================================
    class ChannelStripComponent : public juce::Component
    {
      public:
        ChannelStripComponent(MixerView &owner, PluginProcessor &processor, int channelIndex);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseDoubleClick(const juce::MouseEvent &e) override;

        int getChannelIndex() const noexcept { return channelIndex_; }
        void updateFromState();
        void repaintMeter();
        void setTheme(const Theme &t);

      private:
        MixerView &owner;
        PluginProcessor &processor;
        int channelIndex_ = 0;
        Theme theme = Theme::suite();

        HandButton phaseBtn {"0", true};
        HandButton fxBtn {"FX", true};
        HandButton bleedBtn {"BLD"};
        HandButton soloBtn {"S", true};
        HandButton muteBtn {"M", true};
        HandButton monoBtn {"MONO", true};
        HandButton busBtn {"M 1/2"};
        std::array<juce::Rectangle<float>, InsertChain::slotCount> insertBounds;
        std::unique_ptr<Knob> bleedKnob;

        // Fader & Meter bounds
        juce::Rectangle<float> faderTrackBounds;
        juce::Rectangle<float> faderThumbBounds;
        juce::Rectangle<float> meterBounds;
        float lastDisplayedPeak = -1.0f;

        bool draggingFader = false;
        bool draggingPan = false;
        juce::Rectangle<float> panBarBounds;
    };

    //==========================================================================
    // Factory Bus Strip (Kick / Snare / Room)
    //==========================================================================
    class BusStripComponent : public juce::Component
    {
      public:
        BusStripComponent(MixerView &owner, PluginProcessor &processor, int busIndex);
        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseDoubleClick(const juce::MouseEvent &e) override;
        void setTheme(const Theme &t);

      private:
        MixerView &owner;
        PluginProcessor &processor;
        int busIndex_ = 0;
        Theme theme = Theme::suite();

        juce::Rectangle<float> faderTrackBounds;
        juce::Rectangle<float> faderThumbBounds;
        juce::Rectangle<float> meterBounds;
        juce::Rectangle<float> panBarBounds;
        juce::Rectangle<float> muteBounds;

        bool draggingFader = false;
        bool draggingPan = false;
        HandButton soloBtn {"S", true};
        HandButton muteBtn {"M", true};
        HandButton fxBtn {"FX"};
        std::array<juce::Rectangle<float>, InsertChain::slotCount> insertBounds;
    };

    //==========================================================================
    // Master Strip Component
    //==========================================================================
    class MasterStripComponent : public juce::Component
    {
      public:
        MasterStripComponent(MixerView &owner, PluginProcessor &processor);

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseDoubleClick(const juce::MouseEvent &e) override;

        void updateFromState();
        void repaintMeter();
        void setTheme(const Theme &t);

      private:
        MixerView &owner;
        PluginProcessor &processor;
        Theme theme = Theme::suite();

        HandButton fxBtn {"MASTER FX"};
        HandButton insBtn {"INS"};
        HandButton limiterBtn {"LIMITER", true};

        juce::Rectangle<float> faderTrackBounds;
        juce::Rectangle<float> faderThumbBounds;
        juce::Rectangle<float> meterLBounds;
        juce::Rectangle<float> meterRBounds;
        juce::Rectangle<float> panBarBounds;
        float lastDisplayedPeakL = -1.0f;
        float lastDisplayedPeakR = -1.0f;

        bool draggingFader = false;
        bool draggingPan = false;
    };

    //==========================================================================
    // Subcomponents & Containers
    //==========================================================================
    juce::Viewport channelViewport;
    std::unique_ptr<juce::Component> channelContainer;
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;

    std::unique_ptr<MasterStripComponent> masterStrip;
    std::vector<std::unique_ptr<BusStripComponent>> busStrips;

    // Preset buttons
    HandButton presetCleanBtn {"CLEAN", false};
    HandButton presetRockBtn {"ROCK", false};
    HandButton presetWarmBtn {"WARM", false};
    HandButton presetPunchBtn {"PUNCH", false};
    HandButton presetDryBtn {"DRY", false};
    HandButton presetMixBtn {"MIX READY", false};
    HandButton savePresetBtn {"SAVE .glorpmix"};
    HandButton presetBrowserBtn {"MIXES"};

    // Bleed Matrix Button
    HandButton bleedMatrixBtn {"BLEED MATRIX"};

    // FX Drawers & Popups
    std::unique_ptr<ChannelFXPopup> channelFXPopup;
    std::unique_ptr<InsertRackPopup> insertRackPopup;
    std::unique_ptr<juce::Component> insertPickerLayer;
    std::unique_ptr<juce::Component> insertPickerPanel;
    std::unique_ptr<MasterFXPopup> masterFXPopup;
    std::unique_ptr<PresetBrowserPopup> presetBrowser;
    std::unique_ptr<BleedMatrixPopup> bleedMatrixPopup;

    void applyPreset(int presetIndex);
    void showInsertMenu(bool bus, int channel, int slot, juce::Component *target, juce::Rectangle<int> area);
    void hideInsertPicker();
    void promptSavePreset();

    juce::String activePresetName {"Default Clean"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};

} // namespace glorp
