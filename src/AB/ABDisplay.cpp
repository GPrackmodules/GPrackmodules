#include "plugin.hpp"
#include "AB/ABDisplay.h"

ABDisplay::ABDisplay()
{

}

void ABDisplay::drawLayer(const rack::widget::Widget::DrawArgs& args, int nLayer) /*override*/
{
	if (nLayer== 1)
		draw(args);
	// SvgWidget::drawLayer(args, nLayer);
}

