#pragma once

class ABDisplay : public rack::widget::SvgWidget
{
public:
	ABDisplay() = default;
	void drawLayer(const rack::widget::Widget::DrawArgs& args, int nLayer) override;
};

