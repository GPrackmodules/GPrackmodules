//
// MultiParamChange struct
// =======================
// Write multiple parameter changes to a single UndoRedoAction
//

#include "plugin.h"
#include "MultiParamChange.h"

MultiParamChange::MultiParamChange()
{
	name = "change parameters";
}

void MultiParamChange::undo() //override
{
	for (auto& chg : vChanges)
		chg.undo();
}

void MultiParamChange::redo() //override
{
	for (auto& chg : vChanges)
		chg.redo();
}
