class ErrorModal : public ModalWindowContent
{
	char error[1000];
	virtual void Render(v2 pPosition, RenderState* pState)
	{
		PushText(pState, FONT_Debug, error, pPosition, COLOR_BLACK);
	}
	virtual void OnAccept() { };

	virtual float GetHeight()
	{
		return GetFontSize(FONT_Debug);
	}


public:
	ErrorModal(const char* pError)
	{
		strcpy(error, pError);
	}
};