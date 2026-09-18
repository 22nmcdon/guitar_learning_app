#include "WebUi.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace guitar::app
{

/** The standalone application target.

    Everything platform-specific lives here: the window (or full-screen view on
    mobile) and the app lifecycle. The UI and the engine below it are identical
    on every platform, because they are the same page and the same C++.
*/
class GuitarLearningApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "Guitar Learning App"; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name, juce::Colour (0xfffaf6f0), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new WebUi(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            setResizeLimits (360, 480, 4000, 3000);

            // GUITAR_UI_SIZE=430x860 opens the window at a phone size, so the
            // page's narrow layout can be checked without a device - the page
            // responds to the window the way it responds to a browser window.
            const auto requested = juce::SystemStats::getEnvironmentVariable ("GUITAR_UI_SIZE", {});
            const auto dimensions = juce::StringArray::fromTokens (requested, "x", {});

            if (dimensions.size() == 2 && dimensions[0].getIntValue() > 0)
                centreWithSize (dimensions[0].getIntValue(), dimensions[1].getIntValue());
            else
                centreWithSize (1180, 820);
           #endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace guitar::app

START_JUCE_APPLICATION (guitar::app::GuitarLearningApplication)
