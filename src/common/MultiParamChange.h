//
// MultiParamChange struct
// =======================
// Write multiple parameter changes to a single UndoRedoAction
//

#pragma once

struct MultiParamChange : history::Action
{
	MultiParamChange();

	void undo() override;
	void redo() override;

	vector<history::ParamChange> vChanges;
};