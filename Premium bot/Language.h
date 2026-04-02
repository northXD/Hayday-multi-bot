#pragma once

// Global dil değişkenimiz (Diğer dosyalar da bilsin diye extern yapıyoruz)
extern int g_Language;

// Dil fonksiyonlarımızın tanımları
void AutoDetectLanguage();
const char* Tr(const char* text);