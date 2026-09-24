#include "plugin.h"
#include "AB/ABDisplay.h"

void ABDisplay::drawLayer(const rack::widget::Widget::DrawArgs& args, int nLayer) /*override*/
{
	if (nLayer== 1)
		draw(args);
}

