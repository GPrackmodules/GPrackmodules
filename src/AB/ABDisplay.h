#pragma once

class ABDisplay : public rack::widget::SvgWidget
{
public:
	ABDisplay();

private:
	void drawLayer(const rack::widget::Widget::DrawArgs& args, int nLayer) override;
};

