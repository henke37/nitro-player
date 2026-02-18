#ifndef NINTROCOMPOSER_DEBUGFLAGS_H
#define NINTROCOMPOSER_DEBUGFLAGS_H

namespace NitroComposer {
	struct DebugFlags {
		bool logNotes : 1;
		bool logFlowControl : 1;
		bool logVarWrites : 1;
		bool logCommonEffects : 1;
		bool logUncommonEffects : 1;
		bool logBadData : 1;
	};
}

#endif