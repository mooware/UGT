#include "PlatformPrecomp.h"
#include "GameLogicComponent.h"
#include "App.h"
#include "Entity/EntityUtils.h"
#include "util/MiscUtils.h"
#include "Network/NetUtils.h"
#include "TextAreaComponent.h"
#include "GUIHelp.h"

#include "Renderer/JPGSurfaceLoader.h"
#include "util/utf8.h"

#ifdef _DEBUG
//string g_fileName = "positioning_test.png";
//string g_fileName = "dink.png";
//string g_fileName = "fukuyama.png";
//string g_fileName = "nes_dragon2.png";
//string g_fileName = "space_game.png";
//string g_fileName = "anime_test3.jpg";
//string g_fileName = "temp.jpg";
//string g_fileName = "big_text_test.jpg";
//string g_fileName = "hello_world.png";
//string g_fileName = "nhk_news.png";
string g_fileName;
#else
string g_fileName;
#endif
GameLogicComponent::GameLogicComponent()
{
	SetName("GameLogic");
}

GameLogicComponent::~GameLogicComponent()
{
}


void GameLogicComponent::OnSelected(VariantList* pVList) //0=vec2 point of click, 1=entity sent from
{
	Entity* pEntClicked = pVList->m_variant[1].GetEntity();
	LogMsg("Clicked %s entity at %s", pEntClicked->GetName().c_str(), pVList->m_variant[1].Print().c_str());

	int fingerID = pVList->m_variant[2].GetUINT32();
	//LogMsg("Clicked %s entity at %s", pEntClicked->GetName().c_str(),pVList->m_variant[0].Print().c_str());

	if (pEntClicked->GetName() == "SettingsIcon")
	{
	
		LogMsg("Clicked settings");
		
		/*
		GetApp()->m_sig_kill_all_text();
		GetApp()->SetCaptureMode(CAPTURE_MODE_WAITING);
		GetApp()->m_pGameLogicComp->m_escapiManager.SetPauseCapture(false);
		*/
		Entity *pHelpMenu = CreateHelpMenu(GetParent());
	
	}

	//GetEntityRoot()->PrintTreeAsText(); //useful for debugging
}


void MoveCursorOutOfScreen()
{

	SetCursorPos(GetApp()->m_window_pos_x + GetApp()->m_capture_width,
		GetApp()->m_window_pos_y + GetApp()->m_capture_height);
}

void GameLogicComponent::CreateExamineOverlay()
{
	if (!m_pSettingsIcon)
	{
		m_pSettingsIcon = CreateOverlayButtonEntity(GetEntityRoot(), "SettingsIcon", "interface/settings.rttex", GetScreenSizeXf(), GetScreenSizeYf());
		SetAlignmentEntity(m_pSettingsIcon, ALIGNMENT_DOWN_RIGHT);
		m_pSettingsIcon->GetFunction("OnButtonSelected")->sig_function.connect(1, boost::bind(&GameLogicComponent::OnSelected, this, _1));
		SetTouchPaddingEntity(m_pSettingsIcon, CL_Rectf(0, 0, 0, 0));
	}
}

void GameLogicComponent::KillExamineOverlay()
{
	if (m_pSettingsIcon)
	{
		m_pSettingsIcon->SetTaggedForDeletion();
		m_pSettingsIcon = NULL;
	} 
}
void GameLogicComponent::OnAdd(Entity *pEnt)
{
	EntityComponent::OnAdd(pEnt);

	LogMsg("GameLogic added");
	GetParent()->GetFunction("OnUpdate")->sig_function.connect(1, boost::bind(&GameLogicComponent::OnUpdate, this, _1));
	GetParent()->GetFunction("OnRender")->sig_function.connect(1, boost::bind(&GameLogicComponent::OnRender, this, _1));

	//hack to process a file image on startup, used for testing
	if (!g_fileName.empty())
	{
		KillHelpMenu();

		m_pScreenShot = GetBaseApp()->GetEntityRoot()->GetEntityByName("Screenshot");
		SetOverlayImageEntity(m_pScreenShot, g_fileName);
		
		GetParent()->MoveEntityToBottomByAddress(m_pScreenShot);
		ZoomToPositionFromThisOffsetEntity(m_pScreenShot, CL_Vec2f(0, -GetScreenSizeYf() / 2), 1000, INTERPOLATE_SMOOTHSTEP, 0);
		CL_Vec2f size = GetSize2DEntity(m_pScreenShot);
		assert(size.x != 0 && "Uh, your forced debug filename to load probably doesn't exist");
		GetApp()->m_capture_width = size.x;
		GetApp()->m_capture_height = size.y;
		GetApp()->m_window_pos_x = 0;
		GetApp()->m_window_pos_y = 0;

		GetApp()->SetVideoMode(size.x, size.y, false);

		GetApp()->m_sig_kill_all_text();
		StartProcessingFrameForText();
		GetApp()->SetCaptureMode(CAPTURE_MODE_SHOWING);
		
		AudioHandle handle = GetAudioManager()->Play("audio/wall.mp3");
		GetAudioManager()->SetVol(handle, 0.34f);
		GetApp()->m_hotKeyHandler.OnShowWindow();
	}

	GetAudioManager()->Play("audio/intro.wav");

	if (GetApp()->IsInputDesktop())
	{

	}
	else
	{
		if (!m_escapiManager.Init(GetApp()->m_capture_width, GetApp()->m_capture_height, GetApp()->m_input_camera_device_id))
		{
			LogMsg("Error with camera capture");
		}
	}

	
	
}

bool GameLogicComponent::ReadFromParagraph(const cJSON *paragraph, TextArea &textArea)
{
	//get words
	const cJSON *words = cJSON_GetObjectItemCaseSensitive(paragraph, "words");
	const cJSON *word;
	
	string finalText;

	CL_Vec2f lastVerts[4];
	for (int i = 0; i < 4; i++)
	{
		lastVerts[0] = CL_Vec2f(0, 0);
	}
	
	bool bDidCR = false;
	string lineText;
	string finalTextRaw;

	CL_Rect rectOfLastLine;
	bool bRectSet = false;
	int wordsProcessed = 0;
	float isDialogFuzzyLogic = 0;

	CL_Rect totalRect;
	vector<WordInfo> wordInfo;


	cJSON_ArrayForEach(word, words)
	{
	//	LogMsg("Got a word");
		const cJSON *symbols = cJSON_GetObjectItemCaseSensitive(word, "symbols");
		const cJSON *symbol;

		//get the bounding box of this word
		const cJSON *boundingBox = cJSON_GetObjectItemCaseSensitive(word, "boundingBox");
		const cJSON *vertices = cJSON_GetObjectItemCaseSensitive(boundingBox, "vertices");
		const cJSON *vert;

		CL_Vec2f verts[4];
		int vertCount = 0;
	
		cJSON_ArrayForEach(vert, vertices)
		{
			float x, y;

			cJSON *tempObj = cJSON_GetObjectItem(vert, "x");
			if (tempObj)
			{
				x = tempObj->valuedouble;
			}
			else
			{
				x = 0;
			}
			
			tempObj = cJSON_GetObjectItem(vert, "y");
			if (tempObj)
			{
				y = tempObj->valuedouble;
			}
			else
			{
				y = 0;
			}

			verts[vertCount] = CL_Vec2f(x, y);
			assert(verts[vertCount] >= 0);

			vertCount++;
		}

		
		if (lastVerts[2].y != 0)
		{
			//has valid info, check to see if this word overlaps with the last word drawn in this block.  This is to
			//add the linefeeds which google decides not to include here
			
			float allowedOverlapPixels = 9;
			
			float spaceBetweenWords = verts[0].x - lastVerts[2].x;
			
			if (spaceBetweenWords < (-allowedOverlapPixels))
			{
				
				//new line, it's to the left of the last word
				
#ifdef _DEBUG
				//LogMsg("Rect of %s is %s", lineText.c_str(), PrintRect(rectOfLastLine).c_str());
#endif
				
				LineInfo lineInfo;
				lineInfo.m_lineRect = rectOfLastLine;
				lineInfo.m_words = wordInfo; wordInfo.clear();
				lineInfo.m_text = lineText;
				textArea.m_lines.push_back(lineInfo);
				textArea.m_lineStarts.push_back(rectOfLastLine.get_top_left());
				finalText += lineText;
				finalTextRaw += lineText+" ";
				lineText = "";
				bRectSet = false;
				finalText += "\n";
				bDidCR = true;
			}
		}

		if (!bRectSet)
		{
			//LogMsg("Setting rect for first word");
			if (wordsProcessed == 0)
			{
				rectOfLastLine = CL_Rectf(verts[0].x, verts[0].y, verts[2].x, verts[2].y);
				totalRect = rectOfLastLine;
			}
			else
			{
				rectOfLastLine = CL_Rectf(verts[0].x, verts[0].y, verts[2].x, verts[2].y);
				totalRect.bounding_rect(rectOfLastLine);
			}
			bRectSet = true;
		}
		else
		{
			CL_Rect newWord = CL_Rectf(verts[0].x, verts[0].y, verts[2].x, verts[2].y);

			rectOfLastLine.bounding_rect(newWord);
			totalRect.bounding_rect(newWord);
		}

		for (int i = 0; i < 4; i++)
		{
			lastVerts[i] = verts[i];
		}
		
		if (!lineText.empty())
		{
			lineText += " ";
		}

		bDidCR = false;

		cJSON_ArrayForEach(symbol, symbols)
		{
			const cJSON *text = cJSON_GetObjectItemCaseSensitive(symbol, "text");
			lineText += text->valuestring;

			//what about the exact rect of this text?

			const cJSON *boundingBox2 = cJSON_GetObjectItemCaseSensitive(symbol, "boundingBox");

			vertices = cJSON_GetObjectItemCaseSensitive(boundingBox2, "vertices");
			vertCount = 0;

			cJSON_ArrayForEach(vert, vertices)
			{
				float x, y;

				cJSON *tempObj = cJSON_GetObjectItem(vert, "x");
				
				if (tempObj)
				{
					x = tempObj->valuedouble;
				}
				else
				{
					x = 0;
				}
				
				tempObj = cJSON_GetObjectItem(vert, "y");
				if (tempObj)
				{
					y = tempObj->valuedouble;
				}
				else
				{
					y = 0;
				}

				assert(x >= 0 && y >= 0);
				verts[vertCount] = CL_Vec2f(x, y);
				vertCount++;
			}
			if (vertCount == 4)
			{
				WordInfo w;
				w.m_rect = CL_Rectf(verts[0].x, verts[0].y, verts[2].x, verts[2].y);
				w.m_word = text->valuestring;
				wordInfo.push_back(w);
			} 

		}
	
		wordsProcessed++;
	}

	LineInfo lineInfo;
	lineInfo.m_words = wordInfo; wordInfo.clear();
	
	lineInfo.m_lineRect = rectOfLastLine;
	lineInfo.m_text = lineText;
	textArea.m_lines.push_back(lineInfo);
	textArea.m_lineStarts.push_back(rectOfLastLine.get_top_left());
	finalText += lineText;
	finalTextRaw += lineText;

#ifdef _DEBUG
	//
#endif
	if (!textArea.text.empty()) textArea.text += "\n";
	textArea.text += finalText;
	textArea.rawText += finalTextRaw;
	utf8::utf8to16(finalTextRaw.begin(), finalTextRaw.end(), back_inserter(textArea.wideText));
	textArea.m_rect = totalRect;

	//1 means centered, 0 is far left, 2 is far right
	float centeringFactorX = textArea.m_rect.get_center().x / (GetApp()->m_capture_width / 2);
	float centeringFactorY = textArea.m_rect.get_center().y / (GetApp()->m_capture_height / 2);
	float percentUsedOfScreenWidth = textArea.m_rect.get_width() / GetApp()->m_capture_width;
	float percentUsedOfScreenHeight = textArea.m_rect.get_height() / GetApp()->m_capture_height;

	if (centeringFactorX > 0.8f && percentUsedOfScreenWidth > 0.35f)
	{
		//more likely to be dialog
		isDialogFuzzyLogic += 1.0f;
	}

#ifdef _DEBUG

/*
	LogMsg("Fuzzy tests for %s:", finalText.c_str());
	LogMsg("centeringFactorX: %.2f  centeringFactorY: %.2f", centeringFactorX, centeringFactorY);
	LogMsg("percentUsedOfScreenWidth : %.2f  percentUsedOfScreenHeight: %.2f", percentUsedOfScreenWidth, percentUsedOfScreenHeight);
	*/
#endif
	//recompute the rawtext to look better based on how the text is laid out
	if (textArea.m_lines.size() > 1)
	{
		string newFinal;

		textArea.m_ySpacingToNextLineAverage = 0;
		textArea.m_averageTextHeight = 0;

		for (int i = 0; i < textArea.m_lines.size()-1; i++)
		{
			bool bAddCR = false;

			float startingXDifferenceFromNextLine = textArea.m_lines[i + 1].m_lineRect.get_top_left().x - textArea.m_lines[i].m_lineRect.get_top_left().x;
			float endingXDifferenceFromNextLine = textArea.m_lines[i + 1].m_lineRect.get_top_right().x - textArea.m_lines[i].m_lineRect.get_top_right().x;
			float startingXDiffFromNextLinePercent = startingXDifferenceFromNextLine / textArea.m_rect.get_width();
			float endingXDiffFromNextLinePercent = endingXDifferenceFromNextLine / textArea.m_rect.get_width();
			float xPercentUsedOfTotalRect = textArea.m_lines[i].m_lineRect.get_width() / textArea.m_rect.get_width();
			float ySpacingToNextLine = textArea.m_lines[i + 1].m_lineRect.get_top_left().y -
				textArea.m_lines[i].m_lineRect.get_bottom_right().y;
			float ySpacingToNextLinePercent = ySpacingToNextLine / textArea.m_lines[i + 1].m_lineRect.get_height();

			if (ySpacingToNextLinePercent > 1.9f)
			{
				isDialogFuzzyLogic -= 1; //with huge vertical spacing, unlikely to be dialog
			}

			if (startingXDiffFromNextLinePercent <= 0.05f && ySpacingToNextLinePercent <= 0.35f)
			{
				//text this close together vertically?  Probably dialog
				isDialogFuzzyLogic += 1;
			}

			if (xPercentUsedOfTotalRect < 0.75f)
			{	
				bAddCR = true;
			}
#ifdef _DEBUG
			/*
			LogMsg("ySpacingToNextLinePercent: %.2f startingXDiffFromNextLinePercent: %.2f", ySpacingToNextLinePercent,
				startingXDiffFromNextLinePercent);
				*/
#endif
			newFinal += textArea.m_lines[i].m_text;
			if (bAddCR)
			{
				newFinal += "\n";
			}

			textArea.m_averageTextHeight += textArea.m_lines[i].m_lineRect.get_height();
			textArea.m_ySpacingToNextLineAverage += ySpacingToNextLinePercent;

		}

		textArea.m_averageTextHeight += textArea.m_lines[textArea.m_lines.size() - 1].m_lineRect.get_height();
		textArea.m_ySpacingToNextLineAverage /= (textArea.m_lines.size() - 1);
		textArea.m_averageTextHeight /= textArea.m_lines.size();


		newFinal += textArea.m_lines[textArea.m_lines.size()-1].m_text; //the last line

		textArea.rawText = newFinal;
		//scan if it's dialog or not

		for (int i = 0; i < textArea.wideText.size(); i++)
		{
			if (textArea.wideText[i] == ',' || textArea.wideText[i] == '.' || textArea.wideText[i] == L'�B' ||
				textArea.wideText[i] == L'�A' || textArea.wideText[i] == L'�u')
			{
				isDialogFuzzyLogic += 1;
			}
		}

		if (isDialogFuzzyLogic > 0.5f)
		{
			textArea.m_bIsDialog = true;
		}

	}
	else
	{
		//well, it's just a single line but we still need to set a few things
		textArea.m_averageTextHeight = textArea.m_rect.get_height();
		textArea.m_ySpacingToNextLineAverage = 0.0f;
	}

	//LogMsg("Adding text %s to %s.", textArea.text, PrintRect(textArea.m_rect).c_str());
	return true;
}

void GameLogicComponent::ConstructEntityFromTextArea(TextArea &textArea)
{
	//LogMsg("Constructing %s", textArea.text.c_str());

	Entity *pTextAreaEnt = GetParent()->AddEntity(new Entity("TextArea"));
	TextAreaComponent *pTextAreaComp = (TextAreaComponent*) pTextAreaEnt->AddComponent(new TextAreaComponent());

	pTextAreaComp->Init(textArea);
}

void GameLogicComponent::ConstructEntitiesFromTextAreas()
{
	for (int i = 0; i < m_textareas.size(); i++)
	{
		ConstructEntityFromTextArea(m_textareas[i]);
	}
}

bool GameLogicComponent::BuildDatabase(char *pJson)
{
	LogMsg("Parsing...");

	cJSON *root = cJSON_Parse(pJson);
	
	cJSON *responses = cJSON_GetObjectItem(root, "responses");
	int responseCount = cJSON_GetArraySize(responses);
	if (responseCount != 1) return false;
	
	cJSON *fullTextAnnotation = cJSON_GetObjectItem(cJSON_GetArrayItem(responses, 0), "fullTextAnnotation");
	int fullTextAnnotationCount = cJSON_GetArraySize(fullTextAnnotation);

	cJSON *pages = cJSON_GetObjectItem(fullTextAnnotation, "pages");
	int pagesCount = cJSON_GetArraySize(pages);

	cJSON *blocks = cJSON_GetObjectItem(cJSON_GetArrayItem(pages, 0), "blocks");
	int blockCount = cJSON_GetArraySize(blocks);

	for (int i = 0; i < blockCount; i++)
	{
		TextArea textArea;
		
		cJSON *block = cJSON_GetArrayItem(blocks, i);
		cJSON *boundingBox = cJSON_GetObjectItem(block, "boundingBox");
	
		cJSON *verts = cJSON_GetArrayItem(boundingBox, 0);
		int vertCount = cJSON_GetArraySize(verts);
		assert(vertCount == 4);
		
		for (int j = 0; j < vertCount; j++)
		{
			cJSON *vert = cJSON_GetArrayItem(verts, j);
			float x = 0;
			float y = 0;
			cJSON *tempObj = cJSON_GetObjectItem(vert, "x");
			if (tempObj) x = tempObj->valuedouble;
			tempObj = cJSON_GetObjectItem(vert, "y");
			if (tempObj) y = tempObj->valuedouble;
			//textArea.m_vPoints[j] = CL_Vec2f(x, y);
	

			//LogMsg("Block %d, vert %d is X: %f and Y: %f", i, j, (float)x, (float)y);
		}
		
		//textArea.m_rect = CL_Rectf(textArea.m_vPoints[0].x, textArea.m_vPoints[1].y, textArea.m_vPoints[2].x, textArea.m_vPoints[3].y);
		//LogMsg("Setting final rect to %s", PrintRect(textArea.m_rect).c_str());
		//note:  Here is where I updated my version of cJSON which has some changes...

		//remember what language this block was
		const cJSON *property = cJSON_GetObjectItemCaseSensitive(block, "property");
		const cJSON *detectedLanguages = cJSON_GetObjectItemCaseSensitive(property, "detectedLanguages");
		const cJSON *detectedLanguage;

		cJSON_ArrayForEach(detectedLanguage, detectedLanguages)
		{
			const cJSON *languageCode = cJSON_GetObjectItemCaseSensitive(detectedLanguage, "languageCode");
			textArea.language = languageCode->valuestring;
			//LogMsg("Found language %s", languageCode->valuestring);
		}
		
		const cJSON *paragraph;
		const cJSON *paragraphs = cJSON_GetObjectItemCaseSensitive(block, "paragraphs");
		
		int paraCount = 0;
		cJSON_ArrayForEach(paragraph, paragraphs)
		{
		//	assert(paraCount == 0 && "Should only be one of these!");
			ReadFromParagraph(paragraph, textArea);



			if (textArea.m_rect.get_width() > 5 && textArea.m_rect.get_height() > 5)
				m_textareas.push_back(textArea);
			paraCount++;
		}

		//now let's get the actual letters from the paragraph
#ifdef _DEBUG
		//LogMsg("Got %s", textArea.text.c_str());
		
#endif
		
		assert(textArea.m_rect.left >= 0);
	}

	ConstructEntitiesFromTextAreas();

	cJSON_Delete(root);
	return true; //ok
}

void GameLogicComponent::StartProcessingFrameForText()
{
	GetApp()->m_sig_kill_all_text();
	m_textareas.clear();
	unsigned int originalFileSize = 0;
	byte * fileData = NULL;
	GetApp()->SetGlobalTextHinting(HINTING_NORMAL);

	bool bPlayHide = GetApp()->GetVar("check_hide_overlay")->GetUINT32() != 0;
	if (bPlayHide)
	{
		GetApp()->StartHidingOverlays();
	}


	if (!g_fileName.empty())
	{
		fileData = LoadFileIntoMemoryBasic(g_fileName, &originalFileSize);

	}
	else
	{
		//normal handling

		if (GetApp()->IsInputDesktop())
		{
			m_desktopCapture.Capture(GetApp()->m_window_pos_x, GetApp()->m_window_pos_y, GetApp()->m_capture_width, GetApp()->m_capture_height);
			m_desktopCapture.GetSoftSurface()->FlipY();
			JPGSurfaceLoader jpg;
			jpg.SaveToFile(m_desktopCapture.GetSoftSurface(), "temp.jpg", GetApp()->m_jpg_quality_for_scan);
			m_desktopCapture.GetSoftSurface()->FlipY();

		}
		else
		{

			if (!m_escapiManager.GetSurface()->IsLoaded())
			{
				LogMsg("Can't process, no frame loaded");
				ShowQuickMessage("Can't process, no frame loaded");
				return;
			}

			JPGSurfaceLoader jpg;
			SoftSurface temp;
			temp.Init(m_escapiManager.GetSoftSurface()->GetWidth(), m_escapiManager.GetSoftSurface()->GetHeight(),
				SoftSurface::SURFACE_RGB);
			temp.Blit(0, 0, m_escapiManager.GetSoftSurface());
			temp.FlipY();
			jpg.SaveToFile(&temp, "temp.jpg", GetApp()->m_jpg_quality_for_scan);
		}
		//****************
		fileData = LoadFileIntoMemoryBasic("temp.jpg", &originalFileSize);
	}

	ShowQuickMessage("Analyzing screen...");

	//m_escapiManager.GetSoftSurface()->WriteBMPOut("temp.bmp");


	string postDataOCR_a = R"({
      'requests': [
        {
          'image': {
             'content': ')";

	string postDataOCR_b = R"('
          },
          'features': [
            {
              'type': 'TEXT_DETECTION'
            }
          ]
        }
      ]
    }
)";

	string encodedImage = base64_encode(fileData, originalFileSize);
	
	SAFE_DELETE_ARRAY(fileData);

	string requestWithEmbeddedFile = postDataOCR_a + encodedImage + postDataOCR_b;
	string url = "https://vision.googleapis.com";
	string urlappend = "/v1/images:annotate?key=" + GetApp()->GetGoogleKey();
	m_netHTTP.Setup(url, 80, urlappend, NetHTTP::END_OF_DATA_SIGNAL_HTTP);
	m_netHTTP.AddPostData("", (const byte*)requestWithEmbeddedFile.c_str(), requestWithEmbeddedFile.length());
	m_netHTTP.Start();
}

extern bool g_bHasFocus;

void GameLogicComponent::OnUpdate(VariantList *pVList)
{
	static bool bDidFirstTime = false;

	if (GetApp()->IsInputDesktop())
	{

	}
	else
	{
		m_escapiManager.Update(GetApp()->m_minimum_brightness_for_lumakey);
	}

	if (!bDidFirstTime)
	{
		bDidFirstTime = true;
	}

	m_netHTTP.Update();

	if (m_netHTTP.GetError() != NetHTTP::ERROR_NONE)
	{
		//Big error, show message
		LogMsg("NetHTTP error: %d", m_netHTTP.GetError());
	}

	if (m_netHTTP.GetState() == NetHTTP::STATE_FINISHED)
	{
#ifdef _DEBUG
		FILE *fp = fopen("crap.json", "wb");
		fwrite(m_netHTTP.GetDownloadedData(), m_netHTTP.GetDownloadedBytes(), 1, fp);
		fclose(fp);

#endif

		if (GetApp()->GetCaptureMode() == CAPTURE_MODE_WAITING)
		{
			return; //ignore it, was apparently canceled mid download
		}

		if (!BuildDatabase((char*)m_netHTTP.GetDownloadedData()))
		{
			ShowQuickMessage("Error parsing json reply from google.  View error.txt! Invalid API key?");
			FILE *fp = fopen("error.txt", "wb");
			fwrite(m_netHTTP.GetDownloadedData(), m_netHTTP.GetDownloadedBytes(), 1, fp);
			fclose(fp);
		}

		m_netHTTP.Reset(true);

		if (!GetApp()->IsInputDesktop())
		{
			CreateExamineOverlay();
		}
	}

}

void GameLogicComponent::OnRender(VariantList *pVList)
{

	if (GetApp()->GetCaptureMode() == CAPTURE_MODE_WAITING)
	{
		KillExamineOverlay();
	}

	if ( (GetApp()->m_show_live_video != 0) || GetApp()->GetCaptureMode() == CAPTURE_MODE_SHOWING)
	{

		if (GetApp()->IsInputDesktop())
		{
			//m_desktopCapture.GetSurface()->HardKill();
			//m_desktopCapture.GetSurface()->SetDefaults();

			if ( (!m_desktopCapture.GetSurface()->IsLoaded()) && m_desktopCapture.GetSoftSurface()->IsActive())
			{
				m_desktopCapture.GetSurface()->InitFromSoftSurface(m_desktopCapture.GetSoftSurface());
			}

			if (m_desktopCapture.GetSurface()->IsLoaded())
			{
				m_desktopCapture.GetSurface()->Blit(0, 0);
				DrawRect(GetScreenRect(), MAKE_RGBA(150, 0, 0, 255), 3);
				
				
				CL_Rect r = GetScreenRect();
				r.set_top_left(CL_Vec2i(GetScreenSizeXf() - 300, GetScreenSizeYf() - 20));
				DrawFilledRect(r, MAKE_RGBA(0, 0, 0, 200));

				string msg = "Space to continue - ? for help - rtsoft.com";
					GetApp()->GetFont(FONT_SMALL)->DrawAlignedSolidColor(GetScreenSizeXf()-3, GetScreenSizeYf()-3, msg,
					ALIGNMENT_DOWN_RIGHT, 0.6f, MAKE_RGBA(200, 200, 200, 255));
			}
			else
			{
				//LogMsg("No image");
			}
		}
		else
		{
			if (m_escapiManager.GetSurface()->IsLoaded())
			{
				m_escapiManager.GetSurface()->Blit(0, 0);
			}
		}
		
	}
	else
	{
		//GetApp()->GetFont(FONT_LARGE)->DrawAlignedSolidColor(GetScreenSizeX() / 2, GetScreenSizeYf() - 200, "Universal Game Translator by Seth A. Robinson - rtsoft.com",
		//	ALIGNMENT_DOWN_CENTER, 0.6f);

		//GetApp()->SetSizeForGUIIfNeeded();
	}


	/*
	GetApp()->GetFont(FONT_LARGE)->DrawScaled(20, GetScreenSizeYf() - 200, );
	GetApp()->GetFont(FONT_LARGE)->DrawScaled(20, GetScreenSizeYf() - 100, "" + toString(GetApp()->m_energy), 2.0f, MAKE_RGBA(255, 255, 255, 255));
	*/

}

string MakeFileNameUnique(string fName)
{
	int num = 1;
	char temp[512];
	again:
	sprintf(temp, "%s_%05d.jpg",fName.c_str(), num);
	if (FileExists(temp))
	{
		num++;
		goto again;
	}

	return temp;
}

void GameLogicComponent::OnTakeScreenshot()
{
	//if we are already displaying something, write that out
	string fileName = MakeFileNameUnique("Screenshot");
	GetAudioManager()->Play("audio/snapshot.wav");

	if (GetApp()->GetCaptureMode() == CAPTURE_MODE_SHOWING)
	{
			SoftSurface crap;
			crap.Init(GetScreenSizeX(), GetScreenSizeYf(), SoftSurface::SURFACE_RGB, false);
			crap.BlitFromScreen(0, 0, 0, 0, GetScreenSizeX(), GetScreenSizeYf());
			crap.FlipY();
			JPGSurfaceLoader jpg;
			jpg.SaveToFile(&crap, fileName, GetApp()->m_jpg_quality_for_scan);
	}
	else
	{
		if (!GetApp()->IsInputDesktop())
		{
			//grab and save screenshot from capture device

			if (!m_escapiManager.GetSurface()->IsLoaded())
			{
				LogMsg("Can't process, no frame loaded");
				ShowQuickMessage("Can't process, no frame loaded");
				return;
			}

			JPGSurfaceLoader jpg;
			SoftSurface temp;
			temp.Init(m_escapiManager.GetSoftSurface()->GetWidth(), m_escapiManager.GetSoftSurface()->GetHeight(),
				SoftSurface::SURFACE_RGB);
			temp.Blit(0, 0, m_escapiManager.GetSoftSurface());
			temp.FlipY();
			jpg.SaveToFile(&temp, fileName, GetApp()->m_jpg_quality_for_scan);
		}
	}

	ShowQuickMessage("Saved "+fileName);
}

