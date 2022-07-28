#pragma once
#include "Common/Common.h"

class IMGUIWindow {
public:
	IMGUIWindow() = default;
	virtual ~IMGUIWindow() = default;
	
	virtual bool Init() = 0;
	virtual void Update(float DeltaTime) = 0;

protected:

	std::string m_Name;
	bool m_Open{ true };
};