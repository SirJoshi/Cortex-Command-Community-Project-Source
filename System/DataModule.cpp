#include "DataModule.h"
#include "RTEManagers.h"

namespace RTE {

	const std::string DataModule::m_ClassName = "DataModule";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DataModule::Clear() {
		m_FileName.clear();
		m_FriendlyName.clear();
		m_Author.clear();
		m_Description.clear();
		m_Version = 1;
		m_ModuleID = -1;
		m_IconFile.Reset();
		m_pIcon = 0;
		m_PresetList.clear();
		m_EntityList.clear();
		m_TypeMap.clear();
		for (int i = 0; i < c_PaletteEntriesNumber; ++i) { 
			m_MaterialMappings[i] = 0; 
		}
		m_ScanFolderContents = false;
		m_IgnoreMissingItems = false;
		m_CrabToHumanSpawnRatio = 0;
		m_ScriptPath.clear();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DataModule::Create(std::string moduleName, ProgressCallback fpProgressCallback) {
		m_FileName = moduleName;
		m_ModuleID = g_PresetMan.GetModuleID(moduleName);
		m_CrabToHumanSpawnRatio = 0;

		// Report that we're starting to read a new DataModule
		if (fpProgressCallback) {
			char report[512];
			sprintf_s(report, sizeof(report), "%s %c loading:", m_FileName.c_str(), -43);
			fpProgressCallback(std::string(report), true);
		}

		Reader reader;
		std::string indexPath(m_FileName + "/Index.ini");
		std::string mergedIndexPath(m_FileName + "/MergedIndex.ini");

		// NOTE: This looks for the MergedIndex.ini generated by the index merger tool. The tool is mostly superseded by disabling loading visuals, but still provides some benefit.
		if (std::experimental::filesystem::exists(mergedIndexPath.c_str())) { indexPath = mergedIndexPath; }

		if (std::experimental::filesystem::exists(indexPath.c_str()) && reader.Create(indexPath.c_str(), true, fpProgressCallback) >= 0) {
			int result = Serializable::Create(reader);

			// Print an empty line to separate the end of a module from the beginning of the next one in the loading progress log.
			if (fpProgressCallback) {
				fpProgressCallback(std::string(" "), true);
			}

			// Scan folder contents and load everything *.ini from there
			if (m_ScanFolderContents) {
				al_ffblk fileInfo;
				std::string searchPath = m_FileName + "/" + "*.ini";

				for (int result = al_findfirst(searchPath.c_str(), &fileInfo, FA_ALL); result == 0; result = al_findnext(&fileInfo)) {
					Reader iniReader;
					// Make sure we're not adding Index.ini again
					if (std::strlen(fileInfo.name) > 0 && std::string(fileInfo.name) != "Index.ini") {
						std::string iniPath(m_FileName + "/" + fileInfo.name);
						if (std::experimental::filesystem::exists(iniPath.c_str()) && iniReader.Create(iniPath.c_str(), false, fpProgressCallback) >= 0) {
							result = Serializable::Create(iniReader, false, true);

							// Report loading result
							if (fpProgressCallback) { fpProgressCallback(std::string(" "), true); }
						}
					}
				}
				// Close the file search to avoid memory leaks
				al_findclose(&fileInfo);
			}
			return result;
		}
		return -1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DataModule::Create(const DataModule &reference) {
		RTEAbort("Can't clone Data Modules!"); 
		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DataModule::Destroy(bool notInherited) {
		for (plf::list<PresetEntry>::iterator itr = m_PresetList.begin(); itr != m_PresetList.end(); ++itr) {
			delete (*itr).m_pEntityPreset;
		}
		Clear();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DataModule::ReadModuleProperties(std::string moduleName, ProgressCallback fpProgressCallback) {
		m_FileName = moduleName;
		m_ModuleID = g_PresetMan.GetModuleID(moduleName);
		m_CrabToHumanSpawnRatio = 0;

		// Report that we're starting to read a new DataModule
		if (fpProgressCallback) {
			char report[512];
			sprintf_s(report, sizeof(report), "%s %c reading properties:", m_FileName.c_str(), -43);
			fpProgressCallback(std::string(report), true);
		}
		Reader reader;
		std::string indexPath(m_FileName + "/Index.ini");

		if (std::experimental::filesystem::exists(indexPath.c_str()) && reader.Create(indexPath.c_str(), true, fpProgressCallback) >= 0) {
			reader.SetSkipIncludes(true);
			int result = Serializable::Create(reader);
			return result;
		}
		return -1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DataModule::ReadProperty(std::string propName, Reader &reader) {
		if (propName == "ModuleName") {
			reader >> m_FriendlyName;
		} else if (propName == "Author") {
			reader >> m_Author;
		} else if (propName == "Description") {
			reader >> m_Description;
		} else if (propName == "Version") {
			reader >> m_Version;
		} else if (propName == "ScanFolderContents") {
			reader >> m_ScanFolderContents;
		} else if (propName == "IgnoreMissingItems") {
			reader >> m_IgnoreMissingItems;
		} else if (propName == "CrabToHumanSpawnRatio") {
			reader >> m_CrabToHumanSpawnRatio;
		} else if (propName == "ScriptPath") {
			reader >> m_ScriptPath;
			LoadScripts();
		} else if (propName == "Require") {
			// Check for required dependencies if we're not load properties
			std::string requiredModule;
			reader >> requiredModule;
			if (!reader.GetSkipIncludes() && g_PresetMan.GetModuleID(requiredModule) == -1) {
				reader.ReportError("\"" + m_FileName + "\" requires \"" + requiredModule + "\" in order to load!\n");
			}
		} else if (propName == "IconFile") {
			reader >> m_IconFile;
			m_pIcon = m_IconFile.GetAsBitmap();
		} else if (propName == "AddMaterial") {
			return g_SceneMan.ReadProperty(propName, reader);
		} else {
			// Try to read in the preset and add it to the PresetMan in one go, and report if fail
			if (!g_PresetMan.GetEntityPreset(reader)) { reader.ReportError("Could not understand Preset type!"); }
		}
		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DataModule::Save(Writer &writer) const {
		Serializable::Save(writer);

		writer.NewProperty("ModuleName");
		writer << m_FriendlyName;
		writer.NewProperty("Author");
		writer << m_Author;
		writer.NewProperty("Description");
		writer << m_Description;
		writer.NewProperty("Version");
		writer << m_Version;
		writer.NewProperty("IconFile");
		writer << m_IconFile;

		// TODO: Write out all the different entity instances, each having their own relative location within the data module stored
		// Will need the writer to be able to open different files and append to them as needed, probably done in NewEntity()
		// writer.NewEntity()

		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string DataModule::GetEntityDataLocation(std::string exactType, std::string instance) {
		const Entity *pFoundEntity = GetEntityPreset(exactType, instance);
		if (pFoundEntity == NULL) {
			return 0;
		}

		// Search for entity in instanceList
		for (plf::list<PresetEntry>::iterator itr = m_PresetList.begin(); itr != m_PresetList.end(); ++itr) {
			if ((*itr).m_pEntityPreset == pFoundEntity) {
				return (*itr).m_FileReadFrom;
			}
		}
		RTEAbort("Tried to find allegedly existing Entity Preset Entry: " + pFoundEntity->GetPresetName() + ", but couldn't!");
		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const Entity * DataModule::GetEntityPreset(std::string exactType, std::string instance) {
		if (exactType.empty() || instance == "None" || instance.empty()) {
			return 0;
		}

		std::map<std::string, plf::list<std::pair<std::string, Entity *>>>::iterator classItr = m_TypeMap.find(exactType);
		if (classItr != m_TypeMap.end()) {
			// Find an instance of that EXACT type and name; derived types are not matched
			for (plf::list<std::pair<std::string, Entity *>>::iterator instItr = (*classItr).second.begin(); instItr != (*classItr).second.end(); ++instItr) {
				if ((*instItr).first == instance && (*instItr).second->GetClassName() == exactType) {
					return (*instItr).second;
				}
			}
		}
		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DataModule::AddEntityPreset(Entity *pEntToAdd, bool overwriteSame, std::string readFromFile) {
		// Fail if the entity is unnamed or it's not the original preset.
		//TODO If we're overwriting, we may not want to fail if it's not the original preset, this needs to be investigated
		if (pEntToAdd->GetPresetName() == "None" || pEntToAdd->GetPresetName().empty() || !pEntToAdd->IsOriginalPreset()) {
			return false;
		}
		bool entityAdded = false;
		Entity *pExistingEntity = GetEntityIfExactType(pEntToAdd->GetClassName(), pEntToAdd->GetPresetName());

		if (pExistingEntity) {
			// If we're commanded to overwrite any collisions, then do so by cloning over the existing instance in the list
			// This way we're not invalidating any instance references that would have been taken out and held by clients
			if (overwriteSame) {
				pEntToAdd->SetModuleID(m_ModuleID); //TODO this is probably overwritten by Entity::Create(other), making it useless. Double-check this and remove this line if certain
				pEntToAdd->Clone(pExistingEntity);
				// Make sure the existing one is still marked as the Original Preset
				pExistingEntity->m_IsOriginalPreset = true;
				// Alter the instance entry to reflect the data file location of the new definition
				if (readFromFile != "Same") {
					plf::list<PresetEntry>::iterator itr = m_PresetList.begin();
					for (; itr != m_PresetList.end(); ++itr) {
						// When we find the correct entry, alter its data file location
						if ((*itr).m_pEntityPreset == pExistingEntity) {
							(*itr).m_FileReadFrom = readFromFile;
							break;
						}
					}
					RTEAssert(itr != m_PresetList.end(), "Tried to alter allegedly existing Entity Preset Entry: " + pEntToAdd->GetPresetName() + ", but couldn't find it in the list!");
				}
				return true;
			} else {
				return false;
			}
		} else {
			pEntToAdd->SetModuleID(m_ModuleID);
			Entity *pEntClone = pEntToAdd->Clone();
			// Mark the one we are about to add to the list as the Original now - this is now the actual Original Preset instance
			pEntClone->m_IsOriginalPreset = true;

			if (readFromFile == "Same" && m_PresetList.empty()) {
				RTEAbort("Tried to add first entity instance to data module " + m_FileName + " without specifying a data file!");
			}

			m_PresetList.push_back(PresetEntry(pEntClone, readFromFile != "Same" ? readFromFile : m_PresetList.back().m_FileReadFrom));
			m_EntityList.push_back(pEntClone);
			entityAdded = AddToTypeMap(pEntClone);
			RTEAssert(entityAdded, "Unexpected problem while adding Entity instance \"" + pEntToAdd->GetPresetName() + "\" to the type map of data module: " + m_FileName);
		}
		return entityAdded;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DataModule::GetGroupsWithType(plf::list<std::string> &groupList, std::string withType) {
		bool foundAny = false;

		if (withType == "All" || withType.empty()) {
			for (plf::list<std::string>::const_iterator gItr = m_GroupRegister.begin(); gItr != m_GroupRegister.end(); ++gItr) {
				groupList.push_back(*gItr);
				// TODO: it seems weird that foundAny isn't set to true here, given that the list gets filled. 
				// But I suppose no actual finding is done. Investigate this and see where it's called, maybe this should be changed
			}
		} else {
			std::map<std::string, plf::list<std::pair<std::string, Entity *>>>::iterator classItr = m_TypeMap.find(withType);
			if (classItr != m_TypeMap.end()) {
				const plf::list<std::string> *pGroupList = 0;
				// Go through all the entities of that type, adding the groups they belong to
				for (plf::list<std::pair<std::string, Entity *>>::iterator instItr = classItr->second.begin(); instItr != classItr->second.end(); ++instItr) {
					pGroupList = instItr->second->GetGroupList();
					
					for (plf::list<std::string>::const_iterator gItr = pGroupList->begin(); gItr != pGroupList->end(); ++gItr) {
						groupList.push_back(*gItr); // Get the grouped entities, without transferring ownership
						foundAny = true;
					}
				}

				// Make sure there are no dupe groups in the list
				groupList.sort();
				groupList.unique();
			}
		}
		return foundAny;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DataModule::GetAllOfGroup(plf::list<Entity *> &entityList, std::string group, std::string type) {
		if (group.empty()) {
			return false;
		}

		bool foundAny = false;

		// Find either the Entity typelist that contains all entities in this DataModule, or the specific class' typelist (which will get all derived classes too)
		std::map<std::string, plf::list<std::pair<std::string, Entity *>>>::iterator classItr = m_TypeMap.find((type.empty() || type == "All") ? "Entity" : type);
		
		if (classItr != m_TypeMap.end()) {
			RTEAssert(!classItr->second.empty(), "DataModule has class entry without instances in its map!?");
			for (plf::list<std::pair<std::string, Entity *>>::iterator instItr = classItr->second.begin(); instItr != classItr->second.end(); ++instItr) {
				if (instItr->second->IsInGroup(group)) {
					entityList.push_back(instItr->second); // Get the grouped entities, without transferring ownership
					foundAny = true;
				}
			}
		}
		return foundAny;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DataModule::GetAllOfType(plf::list<Entity *> &entityList, std::string type) {
		if (type.empty()) {
			return false;
		}

		std::map<std::string, plf::list<std::pair<std::string, Entity *>>>::iterator classItr = m_TypeMap.find(type);
		if (classItr != m_TypeMap.end()) {
			RTEAssert(!classItr->second.empty(), "DataModule has class entry without instances in its map!?");

			for (plf::list<std::pair<std::string, Entity *>>::iterator instItr = classItr->second.begin(); instItr != classItr->second.end(); ++instItr) {
				entityList.push_back(instItr->second); // Get the entities, without transferring ownership
			}
			return true;
		}
		return false;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DataModule::AddMaterialMapping(int fromID, int toID) {
		RTEAssert(fromID > 0 && fromID < c_PaletteEntriesNumber && toID > 0 && toID < c_PaletteEntriesNumber, "Tried to make an out-of-bounds Material mapping");

		bool clear = m_MaterialMappings[fromID] == 0;
		m_MaterialMappings[fromID] = toID;

		return clear;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DataModule::LoadScripts() {
		return m_ScriptPath == "" ? 0 : g_LuaMan.RunScriptFile(m_ScriptPath);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DataModule::ReloadAllScripts() {
		for (plf::list<PresetEntry>::iterator itr = m_PresetList.begin(); itr != m_PresetList.end(); ++itr) {
			(*itr).m_pEntityPreset->ReloadScripts();
		}
		LoadScripts();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TODO: This method is almost identical to GetEntityPreset, except it doesn't return a const Entity *. 
	// Investigate if the latter needs to return const (based on what's using it) and if not, get rid of this and replace its uses. At the very least, consider renaming this
	// See https://github.com/cortex-command-community/Cortex-Command-Community-Project-Source/issues/87
	Entity * DataModule::GetEntityIfExactType(const std::string &exactType, const std::string &instanceName) {
		if (exactType.empty() || instanceName == "None" || instanceName.empty()) {
			return 0;
		}

		std::map<std::string, plf::list<std::pair<std::string, Entity *>>>::iterator classItr = m_TypeMap.find(exactType);
		if (classItr != m_TypeMap.end()) {
			// Find an instance of that EXACT type and name; derived types are not matched
			for (plf::list<std::pair<std::string, Entity *>>::iterator instItr = (*classItr).second.begin(); instItr != (*classItr).second.end(); ++instItr) {
				if ((*instItr).first == instanceName && (*instItr).second->GetClassName() == exactType) {
					return (*instItr).second;
				}
			}
		}
		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DataModule::AddToTypeMap(Entity *pEntToAdd) {
		if (!pEntToAdd || pEntToAdd->GetPresetName() == "None" || pEntToAdd->GetPresetName().empty()) {
			return false;
		}

		// Walk up the class hierarchy till we reach the top, adding an entry of the passed in entity into each typelist as we go along
		for (const Entity::ClassInfo *pClass = &(pEntToAdd->GetClass()); pClass != 0; pClass = pClass->GetParent()) {
			std::map<std::string, plf::list<std::pair<std::string, Entity *>>>::iterator classItr = m_TypeMap.find(pClass->GetName());

			// No instances of this entity have been added yet so add a class category for it
			if (classItr == m_TypeMap.end()) {
				classItr = (m_TypeMap.insert(std::pair<std::string, plf::list<std::pair<std::string, Entity *>>>(pClass->GetName(), plf::list<std::pair<std::string, Entity *>>()))).first;
			}

			// NOTE We're adding the entity to the class category list but not transferring ownership. Also, we're not checking for collisions as they're assumed to have been checked for already
			(*classItr).second.push_back(std::pair<std::string, Entity *>(pEntToAdd->GetPresetName(), pEntToAdd));
		}
		return true;
	}
}