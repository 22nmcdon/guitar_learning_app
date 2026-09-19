#include "WebUi.h"

#include "guitar/api/EngineApi.h"

#include <BinaryData.h>

namespace guitar::app
{

using namespace juce;

namespace
{
    // The two directions of the bridge. Names are shared with the page, and
    // nothing else may be sent across it: a rule in one and a drawing in the
    // other is how two shells start disagreeing about what a chord is.
    const Identifier engineCallEvent  { "guitar.engine.call" };
    const Identifier engineReplyEvent { "guitar.engine.reply" };
    const Identifier pageLogEvent     { "guitar.log" };

    /** An argument from the page, as the engine wants it. */
    String argString (const var& args, int index)
    {
        if (auto* array = args.getArray())
            if (isPositiveAndBelow (index, array->size()))
                return (*array)[index].toString();

        return {};
    }

    int argInt (const var& args, int index)
    {
        if (auto* array = args.getArray())
            if (isPositiveAndBelow (index, array->size()))
                return static_cast<int> ((*array)[index]);

        return 0;
    }

    /** Answers one of the engine's questions.

        Named rather than dispatched by table so that an unknown name is an
        answer the page can show, not a silent empty string - which is what a
        page one deploy ahead of its engine would otherwise get.
    */
    std::string answer (const String& name, const var& args)
    {
        const auto text = [&args] (int i) { return argString (args, i).toStdString(); };
        const auto number = [&args] (int i) { return argInt (args, i); };

        if (name == "guitarTunings")        return api::tunings();
        if (name == "guitarChordQualities") return api::chordQualities();
        if (name == "guitarQuizKinds")      return api::quizKinds();
        if (name == "guitarQuizNext")       return api::quizNext();
        if (name == "guitarQuizEnd")        return api::quizEnd();

        if (name == "guitarChordShapes")
            return api::chordShapes (text (0).c_str(), text (1).c_str(),
                                     number (2), number (3), number (4), number (5));

        if (name == "guitarIdentifyShape")
            return api::identifyShape (text (0).c_str(), text (1).c_str());

        if (name == "guitarCheckShape")
            return api::checkShape (text (0).c_str(), text (1).c_str(), text (2).c_str());

        if (name == "guitarFretboardNotes")
            return api::fretboardNotes (text (0).c_str(), number (1), number (2));

        if (name == "guitarPositionsFor")
            return api::positionsFor (text (0).c_str(), text (1).c_str(), number (2), number (3));

        // The quiz these two drive lives in guitar::api, so the app and the
        // browser behave the same way without this shell remembering a thing.
        if (name == "guitarQuizStart")
            return api::quizStart (text (0).c_str(), text (1).c_str(), number (2), number (3),
                                   text (4).c_str(), text (5).c_str(), number (6),
                                   text (7).c_str(), number (8));

        if (name == "guitarProgressMap")
            return api::progressMap (text (0).c_str(), text (1).c_str(),
                                     number (2), number (3), number (4));

        if (name == "guitarQuizAnswer")
            return api::quizAnswer (text (0).c_str(), number (1));

        return "{\"ok\":false,\"error\":\"No engine call named " + name.toStdString() + "\"}";
    }
}

//==============================================================================
WebUi::WebUi()
{
    using Options = WebBrowserComponent::Options;

    auto options = Options {}
        .withNativeIntegrationEnabled()
        .withEventListener (engineCallEvent, [this] (const var& request) { handleEngineCall (request); })
        // Without this a script error in the page is completely silent: there
        // is no console to look at in a shipped app, and the symptom is an
        // interface that simply stops responding.
        .withEventListener (pageLogEvent, [] (const var& report)
        {
            if (auto* object = report.getDynamicObject())
                Logger::writeToLog ("[page] " + object->getProperty ("message").toString()
                                    + " (" + object->getProperty ("line").toString() + ")");
        });

   #if JUCE_WINDOWS
    // Paint the page's cream underneath, so resizing does not flash white.
    options = options.withBackend (Options::Backend::webview2)
                     .withWinWebView2Options (Options::WinWebView2 {}
                                                  .withBackgroundColour (Colour (0xfffaf6f0)));
   #endif

    browser = std::make_unique<WebBrowserComponent> (options);
    addAndMakeVisible (*browser);

    browser->goToURL (pageUrl());
}

WebUi::~WebUi() = default;

void WebUi::resized()
{
    if (browser != nullptr)
        browser->setBounds (getLocalBounds());
}

//==============================================================================
/** Writes the page out and returns a URL for it.

    The page ships inside the binary, but it is handed to the webview as a file
    rather than through JUCE's resource provider. On Linux that provider does
    not reliably deliver a document this size - the page arrives, its CSS paints
    and its script never runs - while the very same page loads perfectly from a
    URL. A file in the app's own temporary directory is the smallest thing that
    behaves the same way on every platform, needs no server and no network, and
    is rewritten on each launch so it can never go stale against the binary.
*/
String WebUi::pageUrl()
{
    int size = 0;
    const auto* data = BinaryData::getNamedResource ("index_html", size);

    if (data == nullptr)
        return {};

    pageFile = File::getSpecialLocation (File::tempDirectory)
                   .getChildFile ("guitar-learning-app-ui")
                   .getChildFile ("index.html");

    pageFile.getParentDirectory().createDirectory();
    pageFile.replaceWithData (data, static_cast<std::size_t> (size));

    return pageFile.getFullPathName().startsWith ("/")
             ? "file://" + pageFile.getFullPathName()
             : pageFile.getFullPathName();
}

void WebUi::handleEngineCall (const var& request)
{
    if (auto* object = request.getDynamicObject())
    {
        const auto id = object->getProperty ("id");
        const auto name = object->getProperty ("name").toString();
        const auto args = object->getProperty ("args");

        auto* reply = new DynamicObject();
        reply->setProperty ("id", id);
        reply->setProperty ("json", String (answer (name, args)));

        if (browser != nullptr)
            browser->emitEventIfBrowserIsVisible (engineReplyEvent, var (reply));
    }
}

} // namespace guitar::app
