#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include <ShlObj.h>

#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"

#include "collections.h"
#include "collections.from_json.h"
#include "autorelease_queue.h"
#include "shared_state.h"
#include "collections.tesregistration.hpp"
#include "collections.tests.hpp"

//#include "skse/SafeWrite.h"


PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

SKSEScaleformInterface		* g_scaleform = NULL;
SKSESerializationInterface	* g_serialization = NULL;
SKSEPapyrusInterface        * g_papyrus = NULL;


/**** serialization ****/

const UInt32 kSerializationDataVersion = 1;

enum {
    kJStorageChunk = 'JSTR',
};

void Serialization_Revert(SKSESerializationInterface * intfc)
{
     collections::shared_state::instance().clearState();
}

void Serialization_Save(SKSESerializationInterface * intfc)
{
	if (intfc->OpenRecord(kJStorageChunk, kSerializationDataVersion)) {
        auto data = collections::shared_state::instance().saveToArray();
        intfc->WriteRecordData(data.data(), data.size());
	}
}

void Serialization_Load(SKSESerializationInterface * intfc)
{
	UInt32	type;
	UInt32	version;
	UInt32	length;
	bool	error = false;

	while(!error && intfc->GetNextRecordInfo(&type, &version, &length))
	{
        if (type == kJStorageChunk && version == kSerializationDataVersion && length > 0) {
            std::vector<char> data(length);
            intfc->ReadRecordData(data.data(), length);
            collections::shared_state::instance().loadAll(data);
            break;
		}
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{
    gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\JContainers.log");
    gLog.SetPrintLevel(IDebugLog::kLevel_Error);
    gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"JContainers";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = skse->GetPluginHandle();

	if (skse->isEditor) {
		_MESSAGE("loaded in editor, marking as incompatible");
		return false;
	}
	else if(skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0) {
		_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);
		return false;
	}
   
	// get the serialization interface and query its version
	g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
	if (!g_serialization) {
		_MESSAGE("couldn't get serialization interface");
		return false;
	}

	if (g_serialization->version < SKSESerializationInterface::kVersion) {
		_MESSAGE("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);
		return false;
	}

    g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);

    if (!g_papyrus) {
        _MESSAGE("couldn't get g_papyrus interface");
        return false;
    }

	return true;
}

bool SKSEPlugin_Load(const SKSEInterface * skse)
{
    _MESSAGE("load");

	// register callbacks and unique ID for serialization
	// ### this must be a UNIQUE ID, change this and email me the ID so I can let you know if someone else has already taken it
	g_serialization->SetUniqueID(g_pluginHandle, kJStorageChunk);

	g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
	g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
	g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

    g_papyrus->Register(collections::registerFuncs);

	return true;
}

__declspec(dllexport) void launchShityTest() {
    testing::runTests(meta<testing::TestInfo>::getListConst());
}

__declspec(dllexport) void produceCode() {
    collections::tes_array::writeSourceToFile();
    collections::tes_map::writeSourceToFile();
    collections::tes_form_map::writeSourceToFile();
    collections::tes_object::writeSourceToFile();
    collections::tes_db::writeSourceToFile();
    collections::tes_jcontainers::writeSourceToFile();
}

};

