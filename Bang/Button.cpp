//
//struct Checkbox
//{
//	bool checked;
//	bool enabled;
//};

////
//// CHECKBOX
////
//static Checkbox CreateCheckbox()
//{
//	Checkbox box;
//	box.checked = false;
//	return box;
//}
//
//static void RenderCheckbox(RenderState* pState, Checkbox* pCheck, v2 pPosition)
//{
//	const float SIZE = 24.0F;
//
//	if (pCheck->enabled)
//	{
//		v2 pos = GetMousePosition() - pPosition;
//		if (pos > V2(0) && pos < V2(SIZE))
//		{
//			if (IsButtonPressed(BUTTON_Left)) pCheck->checked = !pCheck->checked;
//		}
//	}
//
//	PushSizedQuad(pState, pPosition, V2(SIZE), pCheck->enabled ? ELEMENT_BACKGROUND : ELEMENT_BACKGROUND_NOT_ENABLED);
//	if (pCheck->checked)
//	{
//		PushSizedQuad(pState, pPosition + V2(3), V2(18), ACCENT_COLOR);
//	}
//}