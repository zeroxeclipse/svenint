#pragma once

class CImGuiCustom
{
public:
	void Columns(int columns_count, const char* id, bool border, ImVec4 RGBA);
	void EndColumns(ImVec4 RGBA);
	void End();

	void Spacing(float value);
private:

};