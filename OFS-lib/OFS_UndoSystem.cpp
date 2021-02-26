#include "OFS_UndoSystem.h"
#include "FunscriptUndoSystem.h"

#include <array>

// this array provides strings for the StateType enum
// for this to work the order needs to be maintained
static std::array<const std::string, (int32_t)StateType::TOTAL_UNDOSTATE_TYPES> stateStrings = {
	"Add/Edit actions",
	"Add/Edit action",
	"Add action",

	"Remove actions",
	"Remove action",

	"Mouse moved actions",
	"Actions moved",

	"Cut selection",
	"Remove selection",
	"Paste selection",

	"Equalize",
	"Invert",
	"Isolate",

	"Top points",
	"Mid points",
	"Bottom points",

	"Generate actions",
	"Frame align",
	"Range extend",

	"Repeat stroke",

	"Move to current position",

	"Simplify",
	"Lua script",
	"Mono (C#)",
};

const std::string& ScriptState::Message() const
{
	return stateStrings[(int32_t)type];
}

void UndoSystem::Snapshot(StateType type, bool multi_script, Funscript* active, bool clearRedo) noexcept
{
	UndoStack.emplace_back(multi_script); // tracking multi-script modifications

	if (UndoStack.size() > OFS::MaxScriptStateInMemory) {
		UndoStack.erase(UndoStack.begin()); // erase first UndoContext
	}

	// redo gets cleared after every snapshot
	if (clearRedo && !RedoStack.empty())
		ClearRedo();


	if (multi_script) {
		for (auto&& script : *LoadedScripts) {
			script->undoSystem->Snapshot(type, clearRedo);
		}
	}
	else {
		active->undoSystem->Snapshot(type, clearRedo);
	}
}

void UndoSystem::Undo(Funscript* active) noexcept
{
	if (UndoStack.empty()) return;

	if (UndoStack.back().IsMultiscriptModification) {
		for (auto&& script : *LoadedScripts) {
			script->undoSystem->Undo();
		}
	}
	else {
		active->undoSystem->Undo();
	}

	RedoStack.emplace_back(std::move(UndoStack.back()));
	UndoStack.pop_back();
}

void UndoSystem::Redo(Funscript* active) noexcept
{
	if (RedoStack.empty()) return;

	if (RedoStack.back().IsMultiscriptModification) {
		for (auto&& script : *LoadedScripts) {
			script->undoSystem->Redo();
		}
	}
	else {
		active->undoSystem->Redo();
	}
	UndoStack.emplace_back(std::move(RedoStack.back()));
	RedoStack.pop_back();
}

void UndoSystem::ClearRedo() noexcept
{
	RedoStack.clear();
}
