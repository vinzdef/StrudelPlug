/*
  ==============================================================================

    StrudelFetch.h
    Downloads the latest @strudel/repl build into the user's app-data dir on
    first use. Strudel is AGPL and is not redistributed with the plugin.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <functional>

namespace StrudelFetch
{
    static const juce::String cdnBase = "https://cdn.jsdelivr.net/npm/@strudel/repl@latest/dist/";

    inline juce::File directory()
    {
        auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
       #if JUCE_MAC
        base = base.getChildFile ("Application Support");
       #endif
        return base.getChildFile ("StrudelPlug").getChildFile ("strudel");
    }

    // Cached forever once present; delete the directory to force an update.
    inline bool isInstalled()
    {
        return directory().getChildFile ("index.js").existsAsFile();
    }

    inline bool fetchFile (const juce::String& rel, juce::MemoryBlock& out, juce::String& error)
    {
        auto stream = juce::URL (cdnBase + rel).createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs (15000));
        if (stream == nullptr)
        {
            error = "download failed: " + rel;
            return false;
        }
        out.reset();
        stream->readIntoMemoryBlock (out);
        if (out.getSize() == 0)
        {
            error = "empty: " + rel;
            return false;
        }
        return true;
    }

    inline bool writeFile (const juce::String& rel, const juce::MemoryBlock& data, juce::String& error)
    {
        auto file = directory().getChildFile (rel);
        file.getParentDirectory().createDirectory();
        if (file.replaceWithData (data.getData(), data.getSize()))
            return true;
        error = "cannot write " + file.getFullPathName();
        return false;
    }

    // Blocking; run off the message thread.
    inline bool download (juce::String& error)
    {
        juce::MemoryBlock index;
        if (! fetchFile ("index.js", index, error))
            return false;

        // The bundle loads its workers by hashed filename, e.g. assets/clockworker-XXXX.js
        juce::StringArray workers;
        const juce::String js (juce::CharPointer_UTF8 ((const char*) index.getData()), index.getSize());
        for (int pos = 0; (pos = js.indexOf (pos, "assets/")) >= 0; )
        {
            const int end = js.indexOf (pos, ".js");
            if (end < 0) break;
            workers.addIfNotAlreadyThere (js.substring (pos, end + 3));
            pos = end + 3;
        }

        for (const auto& w : workers)
        {
            juce::MemoryBlock data;
            if (! fetchFile (w, data, error) || ! writeFile (w, data, error))
                return false;
        }
        return writeFile ("index.js", index, error);   // last, so isInstalled() implies complete
    }

    // Non-blocking; `done` is called on the message thread.
    inline void downloadAsync (std::function<void (bool ok, juce::String error)> done)
    {
        juce::Thread::launch ([done]
        {
            juce::String error;
            const bool ok = download (error);
            juce::MessageManager::callAsync ([done, ok, error] { done (ok, error); });
        });
    }
}
