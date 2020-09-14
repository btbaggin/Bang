class ErrorModal : public ModalWindowContent
{
	float height;
	char error[1000];
	virtual void Render(v2 pPosition, RenderState* pState)
	{		
		PushText(pState, FONT_Debug, error, pPosition, COLOR_BLACK);
	}
	virtual void OnAccept() { };

	virtual float GetHeight()
	{
		return height;
	}


public:
	ErrorModal(const char* pError)
	{
		float size = GetModalSize(MODAL_SIZE_Half);
		strcpy(error, pError);
		WrapText(error, (u32)strlen(error), FONT_Debug, size);
		height = MeasureString(FONT_Debug, error).Height;
	}
};