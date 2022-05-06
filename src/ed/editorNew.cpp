#include "editorNew.h"
#include "../px/tileset.h"
#include "../rlImGui/rlImGui.h"
#include "../rlImGui/utils.h"
#include "imgui.h"
#include "raylib.h"

void EditorNew::Init()
{

    // Load the settings.
    settings.Load();

    // Then set up the main window.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (settings.maximized ? FLAG_WINDOW_MAXIMIZED : 0));
    InitWindow(settings.width, settings.height, "Kero Master");
    icon = LoadImage("icon.png");
    SetWindowIcon(icon);
    SetTargetFPS(60);
    SetupRLImGui(true);
    style.Load(settings.currStyle);

    // Initialize editor.
    InitEditor();

}

int EditorNew::EditorLoop()
{

    // Main loop.
    while (!WindowShouldClose() || quit)
    {

        // Begin drawing.
        BeginDrawing();

        // Something went wrong, need to show the settings window.
        if (settings.show)
        {
            ClearBackground(fadeColor);
            BeginRLImGui();
            ImGui::OpenPopup("Editor Settings");
            settings.DrawUI(this);
        }

        // Draw like normal.
        else
        {
            Draw();
            BeginRLImGui();
            DrawUI();
        }

        // ImGui demo debug.
        //ImGui::ShowDemoWindow();
        
        // Finish drawing.
        EndRLImGui();
        EndDrawing();
        Update();

        // Fullscreen check.
        /*if (IsKeyPressed(KEY_F4) || doFullscreen)
        {
            if (!IsWindowFullscreen() && !doFullscreen)
            {
                MaximizeWindow();
                timer = GetTime();
                doFullscreen = true;
            }
            if (GetTime() > e.timer + 0.1)
            {
                ToggleFullscreen();
                doFullscreen = false;
            }
        }*/

    }

    // Unload everything.
    UnloadImage(icon);
    ShutdownRLImGui();
    CloseWindow();

    // Nothing wrong, return.
    return 0;

}

void EditorNew::Draw()
{

    // Draw the sub-editors.

    // Safety.
    if (!enabled) return;

    // Draw background.
    map.Clear();
    BeginMode2D(cam);

    // Draw map.
    if (settings.viewLayers[(u8)MapLayer::BG]) map.DrawLayer((u8)MapLayer::BG, tilesets, { 0, 0 }, settings.viewTileAttributes);
    if (settings.viewLayers[(u8)MapLayer::MG]) map.DrawLayer((u8)MapLayer::MG, tilesets, { 0, 0 }, settings.viewTileAttributes);
    if (settings.viewLayers[(u8)MapLayer::FG]) map.DrawLayer((u8)MapLayer::FG, tilesets, { 0, 0 }, settings.viewTileAttributes);

    // Show the map play area.
    if (settings.showPlayArea)
    {
        if (settings.viewLayers[(u8)MapLayer::BG]) DrawRectangleLinesEx({ 0, 0, map.maps[(u8)MapLayer::BG].width * Tileset::EDITOR_TILE_SIZE, map.maps[(u8)MapLayer::BG].height * Tileset::EDITOR_TILE_SIZE }, 1, RED);
        if (settings.viewLayers[(u8)MapLayer::MG]) DrawRectangleLinesEx({ 0, 0, map.maps[(u8)MapLayer::MG].width * Tileset::EDITOR_TILE_SIZE, map.maps[(u8)MapLayer::MG].height * Tileset::EDITOR_TILE_SIZE }, 1, RED);
        if (settings.viewLayers[(u8)MapLayer::FG]) DrawRectangleLinesEx({ 0, 0, map.maps[(u8)MapLayer::FG].width * Tileset::EDITOR_TILE_SIZE, map.maps[(u8)MapLayer::FG].height * Tileset::EDITOR_TILE_SIZE }, 1, RED);
    }

    // Draw tile to place. TODO!!!

    // Draw entity to place. TODO!!!

    // Show the grid.
    DrawGrid();
    EndMode2D();

}

void EditorNew::DrawUI()
{

    // Safety.
    if (!enabled) return;

    // Main menu.
    DrawMainMenu();

    // Level editor.
    levelEditor.DrawUI();

    // Draw select level UI.
    DrawSelectLevelUI();

    // Draw the open level UI.
    DrawOpenLevelUI();

    // About popup.
    DrawAboutPopup();

}

void EditorNew::Update()
{

    // Safety, can't use the editor here.
    if (!enabled)
    {
        FadeEffect();
        return;
    }

    // Get mouse.
    oldMouseX = mouseX;
    oldMouseY = mouseY;
    mouseX = GetMouseX();
    mouseY = GetMouseY();

    // Update tools.
    for (int i = 0; i < (int)ToolItem::NumTools; i++)
    {
        ToolItem item = (ToolItem)i;
        std::vector<int> buttons;
        if (settings.leftClick == item || (currTool == item && settings.leftClick == ToolItem::CurrentTool))
            buttons.push_back(MOUSE_LEFT_BUTTON);
        if (settings.middleClick == item || (currTool == item && settings.middleClick == ToolItem::CurrentTool))
            buttons.push_back(MOUSE_MIDDLE_BUTTON);
        if (settings.rightClick == item || (currTool == item && settings.rightClick == ToolItem::CurrentTool))
            buttons.push_back(MOUSE_RIGHT_BUTTON);
        tools.SetActive(this, i, buttons);
    }
    tools.Update(this);

    // Update focus.
    focus.Update();

}

void EditorNew::FadeEffect()
{
    Vector3 col = ColorToHSV(fadeColor);
    col.x += 0.25;
    if (col.x >= 360)
    {
        col.x = 0;
    }
    fadeColor = ColorFromHSV(col.x, col.y, col.z);
}

void EditorNew::InitEditor()
{
    entityEditor.LoadEntityListing("all");
    LoadFixedTilesets();
    if (settings.rscPath != rsc || settings.lastLevel != level)
    {
        rsc = settings.rscPath;
        level = settings.lastLevel;
        LoadLevel();
    }
}

void EditorNew::LoadTileset(std::string tilesetName)
{
    Tileset t;
    t.Load(rsc, tilesetName);
    tilesets[tilesetName] = t;
}

void EditorNew::UnloadTileset(std::string tilesetName)
{
    if (tilesets.find(tilesetName) != tilesets.end())
    {
        tilesets[tilesetName].Unload();
    }
}

void EditorNew::LoadFixedTilesets()
{
    std::fstream f;
    f.open("object_data/alwaysLoaded.txt", std::ios::in);
    std::string s;
    while (std::getline(f, s))
    {
        LoadTileset(s);
    }
    f.close();
    Tileset::attrTex = LoadTexture("object_data/attribute.png");
    Tileset::unitType = LoadTexture("object_data/unittype.png");
}

void EditorNew::LoadLevel()
{
    if (enabled)
    {
        map.Unload(tilesets);
    }
    cam.offset = { 0, 0 };
    cam.rotation = 0;
    cam.target = { 0, 0 };
    cam.zoom = 2;
    map.Load(rsc, level, tilesets);
    enabled = true;
    settings.lastLevel = level;
    settings.Save();
    undoStack.Reset();
}

void EditorNew::UnloadLevel()
{
    map.Unload(tilesets);
}

void EditorNew::DrawGrid()
{
    if (settings.showGrid)
    {
        int gridWidth = 0;
        int gridHeight = 0;
        for (int i = 0; i < NUM_TILESETS; i++)
        {
            if (settings.viewLayers[i])
            {
                if (map.maps[i].width > gridWidth)
                {
                    gridWidth = map.maps[i].width;
                }
                if (map.maps[i].height > gridHeight)
                {
                    gridHeight = map.maps[i].height;
                }
            }
        }
        const float GRID_ALPHA = 0.1;
        for (int i = 0; i < gridWidth; i++)
        {
            for (int j = 0; j < gridHeight; j++)
            {
                Color c;
                if (i % 2 != j % 2)
                {
                    c = ColorFromNormalized({ 0.2, 0.2, 0.2, GRID_ALPHA });
                }
                else
                {
                    c = ColorFromNormalized({ 0.5, 0.5, 0.5, GRID_ALPHA });
                }
                DrawRectangle(i * Tileset::EDITOR_TILE_SIZE, j * Tileset::EDITOR_TILE_SIZE, Tileset::EDITOR_TILE_SIZE, Tileset::EDITOR_TILE_SIZE, c);
            }
        }
    }
}

bool EditorNew::KeyboardShortcut(bool control, bool alt, bool shift, int key)
{
    bool ret = true;
    ret &= !(control ^ (IsKeyDown(KEY_LEFT_CONTROL) | IsKeyDown(KEY_RIGHT_CONTROL)));
    ret &= !(alt ^ (IsKeyDown(KEY_LEFT_ALT) | IsKeyDown(KEY_RIGHT_ALT)));
    ret &= !(shift ^ (IsKeyDown(KEY_LEFT_SHIFT) | IsKeyDown(KEY_RIGHT_SHIFT)));
    ret &= IsKeyPressed(key);
    return ret;
}

void EditorNew::DrawMainMenu()
{

    // Vars.
    bool openPopup = false;
    bool openSettings = false;
    bool doNew = KeyboardShortcut(true, false, false, KEY_N);
    bool doOpen = KeyboardShortcut(true, false, false, KEY_O);
    bool doSave = KeyboardShortcut(true, false, false, KEY_S);
    bool doSaveAs = KeyboardShortcut(true, false, true, KEY_S);
    bool doClose = KeyboardShortcut(true, false, true, KEY_C);
    bool doQuit = KeyboardShortcut(true, false, true, KEY_Q);
    bool doUndo = undoStack.CanUndo() && KeyboardShortcut(true, false, false, KEY_Z);
    bool doRedo = undoStack.CanRedo() && (KeyboardShortcut(true, false, false, KEY_Y) || KeyboardShortcut(true, false, true, KEY_Z));
    bool isFullscreening = false;
    bool openNewLevelPopup = false;
    bool openAboutPopup = false;

    // File menu.
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
            {
                doNew = true;
            }
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                doOpen = true;
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                doSave = true;
            }
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S"))
            {
                doSaveAs = true;
            }
            if (ImGui::MenuItem("Close", "Ctrl+Shift+C"))
            {
                doClose = true;
            }
            if (ImGui::MenuItem("Quit", "Ctrl+Shift+Q"))
            {
                doQuit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (undoStack.CanUndo() && ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                doUndo = true;
            }

            if (undoStack.CanRedo() && ImGui::MenuItem("Redo", "Ctrl+Y"))
            {
                doRedo = true;
            }

            if (ImGui::MenuItem("Settings"))
            {
                openSettings = true;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Level Editor"))
            {
                levelEditor.open = true;
            }

            if (ImGui::MenuItem("Profile Editor"))
            {
                //profileEditor.open = true;
            }

            if (ImGui::MenuItem("Style Editor"))
            {
                //styleEditor.open = true;
            }

            if (ImGui::MenuItem("Music Player"))
            {
                //musicPlayer.open = true;
            }
            ImGui::EndMenu();
        }
        /* TODO: TOOL SELECT!!!
        if (!startup && ImGui::BeginMenu("Tool"))
        {
            ImGui::RadioButton("Hand (Ctrl+Q)", (int*)&currentTool, (int)EditorTool::Hand);
            ImGuiTooltip("Pan the camera.");
            ImGui::RadioButton("Tile Brush (Ctrl+W)", (int*)&currentTool, (int)EditorTool::TileBrush);
            ImGuiTooltip("Paint tiles.");
            ImGui::RadioButton("Eraser (Ctrl+E)", (int*)&currentTool, (int)EditorTool::Eraser);
            ImGuiTooltip("Erase tiles.");
            ImGui::RadioButton("Entity Hand (Ctrl+R)", (int*)&currentTool, (int)EditorTool::EntityHand);
            ImGuiTooltip("Move, edit, place, and delete entities.");
            ImGui::EndMenu();
        }*/
        if (ImGui::BeginMenu("View"))
        {
            ImGui::Checkbox("Play Area", &settings.showPlayArea);
            ImGui::Checkbox("Show Grid", &settings.showGrid);
            ImGui::Separator();
            ImGui::Checkbox("Show Foreground Layer", &settings.viewLayers[(u8)MapLayer::FG]);
            ImGui::Checkbox("Show Middleground Layer", &settings.viewLayers[(u8)MapLayer::MG]);
            ImGui::Checkbox("Show Background Layer", &settings.viewLayers[(u8)MapLayer::BG]);
            ImGui::Separator();
            ImGui::Checkbox("Entity Images", &settings.viewEntityImages);
            ImGui::Checkbox("Entity Boxes", &settings.viewEntityBoxes);
            ImGui::Checkbox("Entities", &settings.viewEntities);
            ImGui::Separator();
            ImGui::Checkbox("Tile Attributes", &settings.viewTileAttributes);
            /*if (ImGui::Button("Fullscreen"))
            {
                isFullscreening = true;
            }*/
            ImGui::EndMenu();
        }
        /*if (ImGui::MenuItem("Help")) // Help may not be needed.
        {
            helpModal = startup;
            showHelp = true;
        }*/
        if (ImGui::MenuItem("About"))
        {
            openAboutPopup = true;
        }
        ImGui::EndMainMenuBar();
    }

    // Take care of actions.
    if (doNew) LevelNameSelect(false);
    if (doOpen) LevelNameOpen();
    if (doSave) SaveLevel();
    if (doSaveAs) LevelNameSelect(true);
    if (doClose) CloseLevel();
    if (doQuit) Quit();
    if (doUndo) Undo();
    if (doRedo) Redo();
    if (isFullscreening) DoToggleFullscreen();
    if (openAboutPopup) OpenAboutPopup();

}

void EditorNew::LevelNameSelect(bool saveAs)
{
    newFileName = "";
    closeNewLevel = false;
    isNew = !saveAs;
    ImGui::OpenPopup("Enter Level Name");
}

void EditorNew::LevelNameOpen()
{

    // Get new level list and then open popup.
    std::vector<std::string> files = ReadFilesFromDir(rsc + "/field");
    std::sort(files.begin(), files.end());
    if (levelFiles != nullptr) DelImGuiStringList(levelFiles, numLevelFiles);
    levelFiles = GenImGuiStringList(files, &numLevelFiles);
    ImGui::OpenPopup("Select Level");

}

void EditorNew::DrawSelectLevelUI()
{
    if (ImGui::BeginPopupModal("Enter Level Name", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        focus.ObserveFocus();
        focus.isModal |= true;
        ImGuiStringEdit("Level Name", &newFileName);
        if (newFileName != "")
        {
            if (closeNewLevel)
            {
                ImGui::CloseCurrentPopup();
            }
            static Map m;
            if (ImGui::Button("Save"))
            {
                m.levelSettings[0] = 0;
                m.levelSettings[1] = 0;
                m.levelSettings[2] = 0;
                m.levelSettings[3] = 0;
                m.levelSettings[4] = 1;
                m.levelSettings[5] = 0;
                m.levelSettings[6] = 0;
                m.levelSettings[7] = 0;
                m.tilesetSettings1[0] = 2;
                m.tilesetSettings1[1] = 2;
                m.tilesetSettings1[2] = 2;
                m.tilesetSettings2[0] = 0;
                m.tilesetSettings2[1] = 0;
                m.tilesetSettings2[2] = 0;
                bool needsVerify = GFile::FileExists((rsc + "/field/" + newFileName + ".pxpack").c_str());
                if (!needsVerify)
                {
                    if (isNew)
                    {
                        m.Write(rsc, newFileName);
                    }
                    else
                    {
                        map.Write(rsc, newFileName);
                    }
                    UnloadLevel();
                    OpenLevel(newFileName);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ImGui::OpenPopup("Overwrite Level");
                }
            }
            if (ImGui::BeginPopupModal("Overwrite Level", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::Button("Yes"))
                {
                    if (isNew)
                    {
                        m.Write(rsc, newFileName);
                    }
                    else
                    {
                        map.Write(rsc, newFileName);
                    }
                    OpenLevel(newFileName);
                    ImGui::CloseCurrentPopup();
                    closeNewLevel = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("No"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        if (newFileName != "")
        {
            ImGui::SameLine();
        }
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorNew::DrawOpenLevelUI()
{
    if (ImGui::BeginPopupModal("Select Level", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        focus.ObserveFocus();
        focus.isModal |= true;
        ImGui::BeginListBox("Levels", ImVec2(300, 500));
        for (int i = 2; i < numLevelFiles; i++)
        {
            bool dummy = false;
            const char* name = GetFileNameWithoutExt(levelFiles[i]);
            if (ImGui::Selectable(name, &dummy))
            {
                OpenLevel(name);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndListBox();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorNew::DrawAboutPopup()
{
    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        focus.ObserveFocus();
        focus.isModal |= true;
        ImGui::TextColored(ImVec4(.4, 1, 0, 1), "Kero Master");
        ImGui::TextColored(ImVec4(1, .1, .5, 1), "\tAn editor for Kero Blaster, Pink Hour, and Pink Heaven.");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Alula - Windows & SHIFT-JIS support, tile editing, palette, various fixes/improvements.");
        ImGui::TextColored(ImVec4(1, 0, 1, 1), "Gota7 - UI design, format support, entity editing, script editing, tileset editing, editor data.");
        ImGui::Separator();
        if (ImGui::Button("Ok"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorNew::OpenLevel(std::string level)
{
    this->level = level;
    LoadLevel();
}

void EditorNew::SaveLevel()
{
    if (enabled) map.Write(rsc, level);
}

void EditorNew::CloseLevel()
{
    enabled = false;
    map.Unload(tilesets);
}

void EditorNew::Quit()
{
    enabled = false;
    map.Unload(tilesets);
    quit = true;
}

void EditorNew::Undo()
{
    //if (enabled) undoStack.Undo(this);
}

void EditorNew::Redo()
{
    //if (enabled) undoStack.Redo(this);
}

void EditorNew::DoToggleFullscreen()
{
    ToggleFullscreen();
    if (IsWindowFullscreen())
    {
        SetWindowSize(GetMonitorWidth(0), GetMonitorHeight(0));
    }
    else
    {
        SetWindowSize(settings.width, settings.height);
    }
}

void EditorNew::OpenAboutPopup()
{
    ImGui::OpenPopup("About");
}

void EditorNew::OpenTileset(std::string tilesetName)
{

}

void EditorNew::OpenScript(std::string scriptName)
{

}

void EditorNew::MoveCamX(float amount, bool relative)
{
    if (relative)
    {
        cam.offset.x += amount;
    }
    else
    {
        cam.offset.x = amount;
    }
}

void EditorNew::MoveCamY(float amount, bool relative)
{
    if (relative)
    {
        cam.offset.y += amount;
    }
    else
    {
        cam.offset.y = amount;
    }
}

int EditorNew::GetTileX(s8 layer)
{
    int ret = ((mouseX - cam.offset.x) / Tileset::EDITOR_TILE_SIZE / cam.zoom);
    if (ret < 0 || (layer != -1 && ret >= map.maps[layer].width))
    {
        return -1;
    }
    else
    {
        return ret;
    }
}

int EditorNew::GetTileY(s8 layer)
{
    int ret = ((mouseY - cam.offset.y) / Tileset::EDITOR_TILE_SIZE / cam.zoom);
    if (ret < 0 || (layer != -1 && ret >= map.maps[layer].width))
    {
        return -1;
    }
    else
    {
        return ret;
    }
}