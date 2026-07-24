#pragma once

// Selected UI language. -1 means auto-detect on first launch.
extern int g_Language;

void AutoDetectLanguage();
const char* Tr(const char* text);
int GetLanguageCount();
const char* const* GetLanguageNames();
