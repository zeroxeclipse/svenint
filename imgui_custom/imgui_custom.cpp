#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_custom.h"

// Legacy columns can't be set non-resizable or have a proper 'border' color
// unless we edit source code, so i changed those functions and put them here

ImVec4 RGBA{0,0,0,0};

void CImGuiCustom::Columns(int columns_count, const char* id, bool border, ImVec4 Color)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    IM_ASSERT(columns_count >= 1);

    RGBA = Color;

    ImGuiOldColumnFlags flags = (border ? 0 : ImGuiOldColumnFlags_NoBorder | ImGuiOldColumnFlags_NoResize);
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (columns != NULL && columns->Count == columns_count && columns->Flags == flags)
        return;

    if (columns != NULL)
        CImGuiCustom::EndColumns(RGBA);

    if (columns_count != 1)
        ImGui::BeginColumns(id, columns_count, flags);
}

void CImGuiCustom::EndColumns(ImVec4 RGBA)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    ImGui::PopItemWidth();
    if (columns->Count > 1)
    {
        ImGui::PopClipRect();
        columns->Splitter.Merge(window->DrawList);
    }

    const ImGuiOldColumnFlags flags = columns->Flags;

    if (!(flags & ImGuiOldColumnFlags_NoBorder) && !window->SkipItems)
    {
        const float y1 = ImMax(columns->HostCursorPosY, window->ClipRect.Min.y);
        const float y2 = ImMin(window->DC.CursorPos.y, window->ClipRect.Max.y);
        for (int n = 1; n < columns->Count; n++)
        {
            ImGuiOldColumnData* column = &columns->Columns[n];
            float x = window->Pos.x + ImGui::GetColumnOffset(n);
            const ImGuiID column_id = columns->ID + ImGuiID(n);
            const float column_hit_hw = 4.0f;
            const ImRect column_hit_rect(ImVec2(x - column_hit_hw, y1), ImVec2(x + column_hit_hw, y2));
            if (!ImGui::ItemAdd(column_hit_rect, column_id, NULL, ImGuiItemFlags_NoNav))
                continue;

            const ImU32 col = ImGui::GetColorU32(RGBA);
            const float xi = IM_FLOOR(x);
            window->DrawList->AddLine(ImVec2(xi, y1 + 1.0f), ImVec2(xi, y2), col);
        }
    }
}

void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    g.CurrentTable = window && window->DC.CurrentTableIdx != -1 ? g.Tables.GetByIndex(window->DC.CurrentTableIdx) : NULL;
    if (window)
        g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

void CImGuiCustom::End()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (g.CurrentWindowStack.Size <= 1 && g.WithinFrameScopeWithImplicitWindow)
    {
        IM_ASSERT_USER_ERROR(g.CurrentWindowStack.Size > 1, "Calling End() too many times!");
        return;
    }
    IM_ASSERT(g.CurrentWindowStack.Size > 0);

    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        IM_ASSERT_USER_ERROR(g.WithinEndChild, "Must call EndChild() and not End()!");

    if (window->DC.CurrentColumns)
        CImGuiCustom::EndColumns(RGBA);
    ImGui::PopClipRect();   
    ImGui::PopFocusScope();

    if (!(window->Flags & ImGuiWindowFlags_ChildWindow)) 
        ImGui::LogFinish();

    if (window->DC.IsSetPos)
        ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries();

    g.LastItemData = g.CurrentWindowStack.back().ParentLastItemDataBackup;
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
        g.BeginMenuCount--;
    if (window->Flags & ImGuiWindowFlags_Popup)
        g.BeginPopupStack.pop_back();
    g.CurrentWindowStack.back().StackSizesOnBegin.CompareWithCurrentState();
    g.CurrentWindowStack.pop_back();
    SetCurrentWindow(g.CurrentWindowStack.Size == 0 ? NULL : g.CurrentWindowStack.back().Window);
}

// 4: Double Spacing, 8: Triple Spacing, etc...
void CImGuiCustom::Spacing(float value)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;
    ImGui::ItemSize(ImVec2(value, value));
}
