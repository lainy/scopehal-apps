/***********************************************************************************************************************
*                                                                                                                      *
* glscopeclient                                                                                                        *
*                                                                                                                      *
* Copyright (c) 2012-2022 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Katharina B.
	@brief  Stores and manages preference values
 */

#ifndef PreferenceManager_h
#define PreferenceManager_h

#include <map>
#include <string>
#include "PreferenceTree.h"

class PreferenceManager
{
public:
    PreferenceManager()
        : m_treeRoot{ "" }
    {
        DeterminePath();
        InitializeDefaults();
        LoadPreferences();
    }

public:
    // Disallow copy
    PreferenceManager(const PreferenceManager&) = delete;
    PreferenceManager(PreferenceManager&&) = default;

    PreferenceManager& operator=(const PreferenceManager&) = delete;
    PreferenceManager& operator=(PreferenceManager&&) = default;

public:
    void SavePreferences();
    PreferenceCategory& AllPreferences();

    std::string GetConfigDirectory()
    { return m_configDir; }

    // Value retrieval methods
    int64_t GetInt(const std::string& path) const;
    const std::string& GetString(const std::string& path) const;
    double GetReal(const std::string& path) const;
    bool GetBool(const std::string& path) const;
    Gdk::Color GetColor(const std::string& path) const;
    Pango::FontDescription GetFont(const std::string& path) const;

    template< typename E >
    E GetEnum(const std::string& path) const
    {
        return this->GetPreference(path).GetEnum<E>();
    }

private:
    // Internal helpers
    void DeterminePath();
    void InitializeDefaults();
    void LoadPreferences();
    bool HasPreferenceFile() const;
    const Preference& GetPreference(const std::string& path) const;

private:
    PreferenceCategory m_treeRoot;
    std::string m_filePath;
    std::string m_configDir;
};

#endif // PreferenceManager_h
