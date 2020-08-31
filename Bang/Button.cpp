struct Button : UiElement
{
	v2 size;
	v4 color;
	bool clicked;
	char text[10];
	BITMAPS bitmap;
};

static bool IsClicked(Button* pButton, v2 pPosition)
{
	v2 pos = GetMousePosition() - pPosition;
	if (pos.X > 0 && pos.Y > 0 && pos < pButton->size)
	{
		if (IsButtonDown(BUTTON_Left))
		{
			pButton->focused = true;
			pButton->clicked = true;
		}
		else if (IsButtonUp(BUTTON_Left) && pButton->clicked)
		{
			pButton->focused = false;
			pButton->clicked = false;
			return true;
		}
	}
	
	return false;
}

static void RenderButton(RenderState* pState, Button* pButton, v2 pPosition)
{
	bool mouse_over = false;
	bool pressed = false;
	v2 pos = GetMousePosition() - pPosition;
	if (pos.X > 0 && pos.Y > 0 && pos < pButton->size)
	{
		v4 highlight = pButton->color;
		highlight.R = min(highlight.R + 0.2F, 1);
		highlight.G = min(highlight.G + 0.2F, 1);
		highlight.B = min(highlight.B + 0.2F, 1);
		highlight.A = min(highlight.A + 0.2F, 1);
		PushSizedQuad(pState, pPosition - V2(5), pButton->size + V2(10), highlight);
	}

	if (pButton->bitmap != BITMAP_None)
	{
		PushSizedQuad(pState, pPosition, pButton->size, pButton->color, GetBitmap(g_transstate.assets, pButton->bitmap));
	}
	else
	{
		v2 pos = CenterText(FONT_Normal, pButton->text, pButton->size);
		PushSizedQuad(pState, pPosition, pButton->size, pButton->color);
		PushText(pState, FONT_Normal, pButton->text, pPosition + pos, V4(0, 0, 0, 1));
	}
}

static Button CreateButton(v2 pSize, BITMAPS pBitmap, v4 pColor)
{
	Button b = {};
	b.size = pSize;
	b.bitmap = pBitmap;
	b.color = pColor;
	return b;
}

static Button CreateButton(v2 pSize, const char* pText, v4 pColor)
{
	Button b = {};
	b.size = pSize;
	strcpy(b.text, pText);
	b.color = pColor;
	return b;
}

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