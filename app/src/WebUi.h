#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace guitar::app
{

/** The app's user interface: the same page the website serves, in a webview.

    There is one UI in this project and this is how the desktop app gets it. The
    page draws; this owns the process. What the page cannot do for itself it
    asks for over one channel, and today that is exactly one thing - the engine.

    The engine it talks to is the native C++ already in this process, reached
    through guitar::api, not a second copy compiled to WebAssembly. That is why
    building this app needs no Emscripten.

    Deliberately smaller than the equivalent shell in a keyboard app: a guitar
    learner has no MIDI device to open, and the plucked string the page sounds
    is WebAudio, which a webview has exactly as a browser does. A shell that
    owned an audio device here would be a second implementation of something
    that already works in both.
*/
class WebUi : public juce::Component
{
public:
    WebUi();
    ~WebUi() override;

    void resized() override;

private:
    /** Lays the page down where the webview can load it, and says where. */
    juce::String pageUrl();

    /** The page asking the engine a question; the answer goes back by id. */
    void handleEngineCall (const juce::var& request);

    std::unique_ptr<juce::WebBrowserComponent> browser;
    juce::File pageFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebUi)
};

} // namespace guitar::app
