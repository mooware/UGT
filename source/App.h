/*
 *  App.h
 *  Created by Seth Robinson on 3/6/09.
 *  For license info, check the license.txt file that should have come with this.
 *
 */

#pragma once
#include "BaseApp.h"
#include "FreeTypeManager.h"
#include "HotKeyHandler.h"

class GameLogicComponent;
class AutoPlayManager;

enum eViewMode
{
	VIEW_MODE_DEFAULT,
	VIEW_MODE_HIDE_ALL,
	VIEW_MODE_SHOW_SOURCE
};

enum eCaptureMode
{
	CAPTURE_MODE_WAITING,
	CAPTURE_MODE_SHOWING,
	
};

enum eTextHinting
{
	HINTING_NORMAL,
	HINTING_DIALOG,
	HINTING_LINE_BY_LINE
};

class LanguageSetting
{
public:
	string m_languageCode;
	string m_name;
};


class KeyData
{
public:

	string keyName;
	unsigned int virtualKey;
	unsigned int scanCode;
};



class App: public BaseApp
{
public:
	
	App();
	virtual ~App();
	
	bool IsInputDesktop();
	void OnGamepadStickUpdate(VariantList* pVList);
	void UpdateCursor();
	virtual bool Init();
	virtual void Kill();
	virtual void Draw();
	virtual void OnScreenSizeChange();
	virtual void OnEnterBackground();
	virtual void OnEnterForeground();
	bool LoadConfigFile();
	void SetSizeForGUIIfNeeded();
	virtual bool OnPreInitVideo();
	virtual void Update();
	void OnUnloadSurfaces();
	void OnLoadSurfaces();
	bool IsShowingHelp();
	void ScanSubArea();
	void HandleHotKeyPushed(HotKeySetting setting);
	void OnExitApp(VariantList *pVarList);
	string GetGoogleKey() { return m_google_api_key; }
	void StartHidingOverlays();
	void HidingOverlayUpdate();
	bool IsHidingOverlays() { return m_bHidingOverlays; }
	VariantDB* GetShared() { return &m_varDB; }
	Variant* GetVar(const string& keyName);
	Variant* GetVarWithDefault(const string& varName, const Variant& var) { return m_varDB.GetVarWithDefault(varName, var); }


	string m_target_language = "en";
	boost::signal<void(void)> m_sig_target_language_changed;
	boost::signal<void(void)> m_sig_kill_all_text;
	AutoPlayManager* GetAutoPlayManager() { return m_pAutoPlayManager; }

	//we'll wire these to connect to some signals we care about
	void OnAccel(VariantList *pVList);
	void OnArcadeInput(VariantList *pVList);
	FreeTypeManager * GetFreeTypeManager() { return &m_freeTypeManager; }
	eViewMode GetViewMode() { return m_viewMode; }
	void SetViewMode(eViewMode viewMode);
	eCaptureMode GetCaptureMode() { return m_captureMode; }
	void SetCaptureMode(eCaptureMode mode) { m_captureMode = mode; }

	void SetGlobalTextHinting(eTextHinting hint) { m_globalHinting = hint; }
	eTextHinting GetGlobalTextHinting() { return m_globalHinting; }
	HotKeyHandler m_hotKeyHandler;
	HotKeySetting m_hotkey_for_whole_desktop, m_hotkey_for_active_window, m_hotkey_for_draggable_area;
	vector< KeyData> m_keyData;
	eTextHinting m_globalHinting = HINTING_NORMAL;
	//parms from config.txt, I'm lazy so making it all global
	int m_capture_width = 1920;
	int m_capture_height = 1080;
	int m_window_pos_x = 0;
	int m_window_pos_y = 0;
	int m_show_live_video = 0;
	string m_google_api_key;
	int m_jpg_quality_for_scan = 95;
	string m_kanji_lookup_website = "https://jisho.org/search/";
	int m_currentLanguageIndex = -1;
	int m_min_chars_required_to_be_dialog = 8;
	int m_input_camera_device_id = 0;
	int m_minimum_brightness_for_lumakey = 10;
	bool m_audio_stop_when_window_is_closed = false;
	string m_audio_default_language = "ja";
	vector<LanguageSetting> m_languages;
	string m_inputMode = "desktop";
	void SetTargetLanguage(string languageCode, string languageName, bool bShowMessage = true);
	void ModLanguageByIndex(int mod, bool bShowMessage = true);
	void ShowHelp();
	void SetDialogMode();
	void SetLineByLineMode();
	HWND m_forceHWND = 0;
	HWND m_oldHWND = 0;
	VariantDB m_varDB; //holds all data we want to save/load

	bool m_bTestMode = false;
	int m_energy = 0;
	GameLogicComponent *m_pGameLogicComp = NULL;
	eViewMode m_viewMode = VIEW_MODE_DEFAULT;
	bool m_bDidPostInit;
	Surface m_surf; //for testing
	FreeTypeManager m_freeTypeManager;
	eCaptureMode m_captureMode = CAPTURE_MODE_WAITING;
	HotKeySetting GetHotKeyDataFromConfig(string data, string action);
	AutoPlayManager* m_pAutoPlayManager;
	POINT m_hidingOverlayMousePosStart;

	bool m_bHidingOverlays = false;
};


App * GetApp();
const char * GetAppName();
const char * GetBundlePrefix();
const char * GetBundleName();
void ShowQuickMessage(string msg);
