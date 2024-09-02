#include "stdafx.h"
#include "helper.hpp"

#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);

// Version
std::string sFixName = "Disgaea5Fix";
std::string sFixVer = "0.8.0";
std::string sLogFile = sFixName + ".log";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";
std::pair DesktopDimensions = { 0,0 };

// Ini variables
bool bFixRes;
bool bFixAspect;
bool bFixHUD;

// Aspect ratio + HUD stuff
float fPi = (float)3.141592653;
float fAspectRatio;
float fNativeAspect = (float)16 / 9;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fHUDWidthOffset;
float fHUDHeightOffset;

// Variables
int iCurrentResX;
int iCurrentResY;

void CalculateAspectRatio(bool bLog)
{
    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD variables
    fHUDWidth = 720.00f * fAspectRatio;
    fHUDHeight = 720.00f;
    fHUDWidthOffset = (float)(fHUDWidth - 1280.00f) / 2.00f;
    fHUDHeightOffset = 0.00f;
    if (fAspectRatio < fNativeAspect) {
        fHUDWidth = 1280.00f;
        fHUDHeight = 1280.00f / fAspectRatio;
        fHUDWidthOffset = 0.00f;
        fHUDHeightOffset = (float)(fHUDHeight - 720.00f) / 2.00f;
    }

    if (bLog) {
        // Log details about current resolution
        spdlog::info("----------");
        spdlog::info("Current Resolution: Resolution: {}x{}", iCurrentResX, iCurrentResY);
        spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
        spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
        spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
        spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
        spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
        spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
        spdlog::info("----------");
    }
}

void Logging()
{
    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sExePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Path to logfile: {}", sExePath.string() + sLogFile);
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex) {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
            FreeLibraryAndExitThread(baseModule, 1);
        }
    }
}

void Configuration()
{
    // Initialise config
    std::ifstream iniFile(sExePath.string() + sConfigFile);
    if (!iniFile) {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sExePath.string().c_str() << std::endl;
        FreeLibraryAndExitThread(baseModule, 1);
    }
    else {
        spdlog::info("Path to config file: {}", sExePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();

    inipp::get_value(ini.sections["Fix Resolution"], "Enabled", bFixRes);
    spdlog::info("Config Parse: bFixRes: {}", bFixRes);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bFixAspect);
    spdlog::info("Config Parse: bFixAspect: {}", bFixAspect);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    spdlog::info("Config Parse: bFixHUD: {}", bFixHUD);

    // Grab desktop resolution/aspect
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();
    iCurrentResX = DesktopDimensions.first;
    iCurrentResY = DesktopDimensions.second;
    CalculateAspectRatio(false);
}

void Resolution()
{
    if (bFixRes) {
        // Stop resolution from being scaled to 16:9
        uint8_t* ResolutionResizeScanResult = Memory::PatternScan(baseModule, "7E ?? 85 ?? 79 ?? ?? 39 8E E3 38 41 ?? ?? 44 ?? ?? 41 ?? ?? 41 ?? ?? C1 ?? 1F 44 ?? ?? 41 ?? ?? 8B ?? 99");
        if (ResolutionResizeScanResult) {
            spdlog::info("Resolution Resize: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ResolutionResizeScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)ResolutionResizeScanResult, "\xEB", 1);
            spdlog::info("Resolution Resize: Patched instruction.");
        }
        else if (!ResolutionResizeScanResult) {
            spdlog::error("Resolution Resize: Pattern scan failed.");
        }
    }

    // Get current resolution
    uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "8B ?? 99 33 ?? 2B ?? 83 ?? ?? ?? ?? 85 ?? 79 ?? ?? 39 8E E3 38 41 ?? ?? 44 ?? ?? 41 ?? ?? 41 ?? ?? C1 ?? 1F 44 ?? ?? 41 ?? ?? 8B ?? 99");
    if (CurrResolutionScanResult) {
        spdlog::info("Current Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CurrResolutionScanResult - (uintptr_t)baseModule);

        static SafetyHookMid CurrResolutionMidHook{};
        CurrResolutionMidHook = safetyhook::create_mid(CurrResolutionScanResult,
            [](SafetyHookContext& ctx) {
                // Get ResX and ResY
                int iResX = (int)ctx.rdi;
                int iResY = (int)ctx.r11;

                // Only log on resolution change
                if (iResX != iCurrentResX || iResY != iCurrentResY) {
                    iCurrentResX = iResX;
                    iCurrentResY = iResY;
                    CalculateAspectRatio(true);
                }
            });
    }
    else if (!CurrResolutionScanResult) {
        spdlog::error("Current Resolution: Pattern scan failed.");
    }
}

void AspectRatio()
{
    if (bFixAspect) {
        // 3D Aspect Ratio
        uint8_t* AspectRatioScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 33 ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ??");
        if (AspectRatioScanResult) {
            spdlog::info("Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)AspectRatioScanResult - (uintptr_t)baseModule);

            static SafetyHookMid AspectRatioMidHook{};
            AspectRatioMidHook = safetyhook::create_mid(AspectRatioScanResult + 0x23,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio != fNativeAspect) {
                        ctx.xmm1.f32[0] = fAspectRatio;
                    }
                });          
        }
        else if (!AspectRatioScanResult) {
            spdlog::error("Aspect Ratio: Pattern scan failed.");
        }
    }
}

void HUD()
{
    if (bFixHUD) {
        // HUD Scale
        uint8_t* HUDScaleScanResult = Memory::PatternScan(baseModule, "0F 85 ?? ?? ?? ?? F3 41 ?? ?? ?? ?? F3 41 ?? ?? ?? ?? ?? ?? 00 F3 0F ?? ??");
        if (HUDScaleScanResult) {
            spdlog::info("HUD: HUD Scale: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDScaleScanResult - (uintptr_t)baseModule);

            if (bFixAspect) {
                static SafetyHookMid HUDScaleMidHook{};
                HUDScaleMidHook = safetyhook::create_mid(HUDScaleScanResult + 0x6,
                    [](SafetyHookContext& ctx) {
                        if (ctx.r11 + 0x19C) {
                            *reinterpret_cast<float*>(ctx.r11 + 0x19C) = 1.00f / 1280.00f; // Default
                            *reinterpret_cast<float*>(ctx.r11 + 0x1A0) = 1.00f / -720.00f; // Default

                            if (fAspectRatio > fNativeAspect) {
                                *reinterpret_cast<float*>(ctx.r11 + 0x19C) = 1.00f / (720.00f * fAspectRatio);
                            }
                            else if (fAspectRatio < fNativeAspect) {
                                *reinterpret_cast<float*>(ctx.r11 + 0x1A0) = 1.00f / (-1280.00f / fAspectRatio);
                            }
                        }
                    });
            }
        }
        else if (!HUDScaleScanResult) {
            spdlog::error("HUD: HUD Scale: Pattern scan failed.");
        }
        
        // Sprite Positions
        uint8_t* SpritePositionsScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 44 0F ?? ?? ?? ?? ?? ?? ?? F3 0F ?? ?? 66 0F ?? ??");
        if (SpritePositionsScanResult) {
            spdlog::info("HUD: Character Sprite Positions: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)SpritePositionsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid SpritePositionsMidHook{};
            SpritePositionsMidHook = safetyhook::create_mid(SpritePositionsScanResult + 0x10,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.xmm0.f32[0] /= 640.00f;
                        ctx.xmm0.f32[0] *= 360.00f * fAspectRatio;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.xmm1.f32[0] /= -360.00f;
                        ctx.xmm1.f32[0] *= -640.00f / fAspectRatio;
                    }
                });
        }
        else if (!SpritePositionsScanResult) {
            spdlog::error("HUD: Character Sprite Positions: Pattern scan failed.");
        }

        // Health Bar Positions
        uint8_t* HealthBarPositionsScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? E8 ?? ?? ?? ??");
        if (HealthBarPositionsScanResult) {
            spdlog::info("HUD: Health Bar Positions: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HealthBarPositionsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HealthBarPositionsMidHook{};
            HealthBarPositionsMidHook = safetyhook::create_mid(HealthBarPositionsScanResult + 0x16,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.xmm3.f32[0] /= 640.00f;
                        ctx.xmm3.f32[0] *= 360.00f * fAspectRatio;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.xmm1.f32[0] /= 360.00f;
                        ctx.xmm1.f32[0] *= 640.00f / fAspectRatio;
                    }
                });
        }
        else if (!HealthBarPositionsScanResult) {
            spdlog::error("HUD: Health Bar Positions: Pattern scan failed.");
        }

        // Sprite Draw Distance
        uint8_t* SpriteDrawDistancesScanResult = Memory::PatternScan(baseModule, "F3 44 ?? ?? ?? F3 0F ?? ?? F3 45 ?? ?? ?? F3 0F ?? ?? F3 44 ?? ?? ?? 41 ?? ?? ?? E8 ?? ?? ?? ??");
        if (SpriteDrawDistancesScanResult) {
            spdlog::info("HUD: Character Sprite Draw Distance: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)SpriteDrawDistancesScanResult - (uintptr_t)baseModule);

            static SafetyHookMid SpriteDrawDistancesMidHook{};
            SpriteDrawDistancesMidHook = safetyhook::create_mid(SpriteDrawDistancesScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.xmm2.f32[0] *= fAspectMultiplier;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.xmm1.f32[0] *= fAspectMultiplier;
                    }
                });
        }
        else if (!SpriteDrawDistancesScanResult) {
            spdlog::error("HUD: Character Sprite Draw Distance: Pattern scan failed.");
        }

        // Glpyhs Position
        uint8_t* GlyphPositionScanResult = Memory::PatternScan(baseModule, "41 ?? ?? 8B ?? 83 ?? ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? ?? BA 01 00 00 00 0F 29 ?? ?? ?? 0F 29 ?? ?? ?? 44 0F ?? ?? ?? ??");
        if (GlyphPositionScanResult) {
            spdlog::info("HUD: Glyph Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GlyphPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GlyphPositionMidHook{};
            GlyphPositionMidHook = safetyhook::create_mid(GlyphPositionScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.r8 += (int)fHUDHeightOffset;
                    }
                });
        }
        else if (!GlyphPositionScanResult) {
            spdlog::error("HUD: Glyph Position: Pattern scan failed.");
        }

        // Text Position
        uint8_t* TextPositionScanResult = Memory::PatternScan(baseModule, "41 ?? ?? 8B ?? 83 ?? ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? ?? BA 01 00 00 00 0F 29 ?? ?? ?? 0F 29 ?? ?? ?? 44 0F ?? ?? ?? ??");
        if (TextPositionScanResult) {
            spdlog::info("HUD: Text Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)TextPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid TextPositionMidHook{};
            TextPositionMidHook = safetyhook::create_mid(TextPositionScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.r8 += (int)fHUDHeightOffset;
                    }
                });
        }
        else if (!TextPositionScanResult) {
            spdlog::error("HUD: Text Position: Pattern scan failed.");
        }

        // Highlights Position
        uint8_t* HighlightPositionScanResult = Memory::PatternScan(baseModule, "41 8B ?? 8B ?? 83 ?? ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? ??");
        if (HighlightPositionScanResult) {
            spdlog::info("HUD: Highlight Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HighlightPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HighlightPositionMidHook{};
            HighlightPositionMidHook = safetyhook::create_mid(HighlightPositionScanResult,
                [](SafetyHookContext& ctx) {                
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.r8 += (int)fHUDHeightOffset;
                    }

                    // Battle Backgrounds
                    if (ctx.rsp + 0x90) {
                        if (*reinterpret_cast<int*>(ctx.rsp + 0x90) == 1281 && *reinterpret_cast<int*>(ctx.rsp + 0x98) == 721)
                        {
                            if (fAspectRatio > fNativeAspect) {
                                *reinterpret_cast<int*>(ctx.rsp + 0x90) = (int)ceilf(721.00f * fAspectRatio);
                            }
                            else if (fAspectRatio < fNativeAspect) {
                                *reinterpret_cast<int*>(ctx.rsp + 0x98) = (int)ceilf(1281.00f / fAspectRatio);
                            }
                            ctx.rdx = ctx.r8 = 0;
                        }
                    }
                });
        }
        else if (!HighlightPositionScanResult) {
            spdlog::error("HUD: Highlight Position: Pattern scan failed.");
        }

        // Bars Position
        uint8_t* BarsPositionScanResult = Memory::PatternScan(baseModule, "41 ?? ?? 8B ?? 83 ?? ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? ?? BA 01 00 00 00");
        if (BarsPositionScanResult) {
            spdlog::info("HUD: Bars Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)BarsPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid BarsPositionMidHook{};
            BarsPositionMidHook = safetyhook::create_mid(BarsPositionScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.r8 += (int)fHUDHeightOffset;
                    }                
                });
        }
        else if (!BarsPositionScanResult) {
            spdlog::error("HUD: Bars Position: Pattern scan failed.");
        }

        // Images Position
        uint8_t* ImagesPositionScanResult = Memory::PatternScan(baseModule, "41 ?? ?? 8B ?? 83 ?? ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? ?? BA 01 00 00 00");
        if (ImagesPositionScanResult) {
            spdlog::info("HUD: Images Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ImagesPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid ImagesPositionMidHook{};
            ImagesPositionMidHook = safetyhook::create_mid(ImagesPositionScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.r8 += (int)fHUDHeightOffset;
                    }                   
                });
        }
        else if (!ImagesPositionScanResult) {
            spdlog::error("HUD: Images Position: Pattern scan failed.");
        }

        // Ticker Position
        uint8_t* TickerPositionScanResult = Memory::PatternScan(baseModule, "41 ?? ?? 8B ?? 41 ?? ?? ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? ?? 48 89 ?? ??");
        if (TickerPositionScanResult) {
            spdlog::info("HUD: Ticker Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)TickerPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid TickerPositionMidHook{};
            TickerPositionMidHook = safetyhook::create_mid(TickerPositionScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        ctx.r8 += (int)fHUDHeightOffset;
                    }

                    // Fade to black
                    if (ctx.rax + 0x28) {
                        if (*reinterpret_cast<int*>(ctx.rax + 0x28) == 1280  && *reinterpret_cast<int*>(ctx.rax + 0x30) == 720)
                        {
                            if (fAspectRatio > fNativeAspect) {
                                *reinterpret_cast<int*>(ctx.rax + 0x28) = (int)ceilf(720.00f * fAspectRatio);
                            }
                            else if (fAspectRatio < fNativeAspect) {
                                *reinterpret_cast<int*>(ctx.rax + 0x30) = (int)ceilf(1280.00f / fAspectRatio);
                            }
                            ctx.rdx = ctx.r8 = 0;
                        }
                    }
                });
        }
        else if (!TickerPositionScanResult) {
            spdlog::error("HUD: Ticker Position: Pattern scan failed.");
        }

        // Logos Position
        uint8_t* LogosPositionScanResult = Memory::PatternScan(baseModule, "F3 44 ?? ?? ?? F3 0F ?? ?? F3 44 ?? ?? ?? F3 44 ?? ?? ?? F3 0F ?? ?? F3 44 ?? ?? ?? 41 0F ?? ??");
        if (LogosPositionScanResult) {
            spdlog::info("HUD: Logos Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)LogosPositionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid LogosPositionMidHook{};
            LogosPositionMidHook = safetyhook::create_mid(LogosPositionScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.xmm8.f32[0] += fHUDWidthOffset;
                        ctx.xmm6.f32[0] /= fAspectMultiplier;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        // TODO
                    }
                });
        }
        else if (!LogosPositionScanResult) {
            spdlog::error("HUD: Logos Position: Pattern scan failed.");
        }

        // Ticker Clipping
        uint8_t* TickerClippingScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? BA ?? ?? ?? ?? C7 44 ?? ?? ?? ?? ?? ?? C7 44 ?? ?? 01 ?? ?? ??");
        if (TickerClippingScanResult) {
            spdlog::info("HUD: Ticker Clipping: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)TickerClippingScanResult - (uintptr_t)baseModule);

            static SafetyHookMid TickerClippingLeftMidHook{};
            TickerClippingLeftMidHook = safetyhook::create_mid(TickerClippingScanResult - 0x3E,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rdi += (int)fHUDWidthOffset;
                        ctx.rdx += (int)fHUDWidthOffset;
                    }
                });

            static SafetyHookMid TickerClippingRightMidHook{};
            TickerClippingRightMidHook = safetyhook::create_mid(TickerClippingScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.rbx += (int)fHUDWidthOffset;
                    }
                });
        }
        else if (!TickerClippingScanResult) {
            spdlog::error("HUD: Ticker Clipping: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    Resolution();
    AspectRatio();
    HUD();
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}